#include "psi.h"

// Symbol server path: srv*<downstream cache>*<upstream HTTPS url>. The cache
// sits under C:\Windows\Temp because it's world-writable on a default install;
// if the directory can't be created the whole SymSrv path silently falls back
// to "<unavailable>" rather than erroring.
#define PSI_SYM_SEARCH_PATH \
    "srv*C:\\Windows\\Temp\\psi_syms*https://msdl.microsoft.com/download/symbols"

#define PSI_EAT_MAX_ENTRIES 100000u

static PPSI_LDR_DATA_TABLE_ENTRY psi_find_backing_module(unsigned long long addr) {
    PPEB peb = (PPEB)__readgsqword(0x60);
    if (!peb || !peb->Ldr) return NULL;
    PLIST_ENTRY head = &peb->Ldr->InMemoryOrderModuleList;

    for (PLIST_ENTRY cur = head->Flink; cur && cur != head; cur = cur->Flink) {
        PPSI_LDR_DATA_TABLE_ENTRY e =
            CONTAINING_RECORD(cur, PSI_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        unsigned long long base = (unsigned long long)(uintptr_t)e->DllBase;
        unsigned long long size = (unsigned long long)e->SizeOfImage;
        if (base == 0 || size == 0) continue;
        if (base + size < base) continue;
        if (addr >= base && addr < base + size) return e;
    }
    return NULL;
}

static IMAGE_NT_HEADERS64 *psi_nt_headers(PVOID base) {
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    IMAGE_NT_HEADERS64 *nt =
        (IMAGE_NT_HEADERS64 *)((BYTE *)base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;
    return nt;
}

static BOOL psi_section_for_rva(
    IMAGE_NT_HEADERS64 *nt, DWORD rva,
    char out_name[9], DWORD *out_start, DWORD *out_end, DWORD *out_char,
    BOOL *in_headers)
{
    *in_headers = FALSE;
    WORD nsec = nt->FileHeader.NumberOfSections;
    if (nsec == 0 || nsec > 96) return FALSE;

    PIMAGE_SECTION_HEADER sh = IMAGE_FIRST_SECTION(nt);

    DWORD first_va = sh[0].VirtualAddress;
    if (rva < first_va) {
        out_name[0] = '<'; out_name[1] = 'h'; out_name[2] = 'e';
        out_name[3] = 'a'; out_name[4] = 'd'; out_name[5] = 'r';
        out_name[6] = 's'; out_name[7] = '>'; out_name[8] = 0;
        *out_start = 0;
        *out_end   = first_va;
        *out_char  = 0;
        *in_headers = TRUE;
        return TRUE;
    }

    for (WORD i = 0; i < nsec; i++) {
        DWORD va = sh[i].VirtualAddress;
        DWORD sz = sh[i].Misc.VirtualSize;
        if (sz == 0) sz = sh[i].SizeOfRawData;
        if (va + sz < va) continue;
        if (rva >= va && rva < va + sz) {
            for (int j = 0; j < 8; j++) out_name[j] = (char)sh[i].Name[j];
            out_name[8] = 0;
            *out_start = va;
            *out_end   = va + sz;
            *out_char  = sh[i].Characteristics;
            return TRUE;
        }
    }
    return FALSE;
}

static void psi_print_section_perm(formatp *fb, DWORD ch) {
    char r = (ch & IMAGE_SCN_MEM_READ)    ? 'R' : '-';
    char w = (ch & IMAGE_SCN_MEM_WRITE)   ? 'W' : '-';
    char x = (ch & IMAGE_SCN_MEM_EXECUTE) ? 'X' : '-';
    FPRINT(fb, "%c%c%c", r, w, x);
}

static BOOL psi_nearest_eat_export(
    PVOID base, DWORD size_of_image, DWORD target_rva,
    char out_name[256], DWORD *out_offset)
{
    IMAGE_NT_HEADERS64 *nt = psi_nt_headers(base);
    if (!nt) return FALSE;

    IMAGE_DATA_DIRECTORY *dir =
        &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (dir->VirtualAddress == 0 || dir->Size == 0) return FALSE;
    if (dir->VirtualAddress >= size_of_image) return FALSE;
    if ((DWORD64)dir->VirtualAddress + dir->Size > size_of_image) return FALSE;

    IMAGE_EXPORT_DIRECTORY *exp =
        (IMAGE_EXPORT_DIRECTORY *)((BYTE *)base + dir->VirtualAddress);
    if (exp->NumberOfFunctions > PSI_EAT_MAX_ENTRIES) return FALSE;
    if (exp->NumberOfNames     > PSI_EAT_MAX_ENTRIES) return FALSE;
    if (exp->AddressOfFunctions    >= size_of_image) return FALSE;
    if (exp->AddressOfNames        >= size_of_image) return FALSE;
    if (exp->AddressOfNameOrdinals >= size_of_image) return FALSE;

    DWORD *funcs = (DWORD *)((BYTE *)base + exp->AddressOfFunctions);
    DWORD *names = (DWORD *)((BYTE *)base + exp->AddressOfNames);
    WORD  *ords  = (WORD  *)((BYTE *)base + exp->AddressOfNameOrdinals);

    DWORD exp_lo = dir->VirtualAddress;
    DWORD exp_hi = dir->VirtualAddress + dir->Size;

    BOOL  have_best    = FALSE;
    DWORD best_rva     = 0;
    DWORD best_name_rv = 0;

    for (DWORD i = 0; i < exp->NumberOfNames; i++) {
        WORD ord = ords[i];
        if (ord >= exp->NumberOfFunctions) continue;
        DWORD fn_rva = funcs[ord];
        if (fn_rva == 0) continue;
        if (fn_rva >= exp_lo && fn_rva < exp_hi) continue; // forwarder
        if (fn_rva > target_rva) continue;
        if (!have_best || fn_rva > best_rva) {
            best_rva     = fn_rva;
            best_name_rv = names[i];
            have_best    = TRUE;
        }
    }

    if (!have_best) return FALSE;
    if (best_name_rv >= size_of_image) return FALSE;

    const char *nm = (const char *)((BYTE *)base + best_name_rv);
    size_t i = 0;
    while (i < 255 && nm[i] >= 0x20 && nm[i] <= 0x7e) {
        out_name[i] = nm[i];
        i++;
    }
    out_name[i] = 0;
    *out_offset = target_rva - best_rva;
    return TRUE;
}

static BOOL psi_sym_lookup(
    unsigned long long addr, PPSI_LDR_DATA_TABLE_ENTRY mod,
    char *out_name, size_t name_cap, unsigned long long *out_disp)
{
    if (!mod) return FALSE;

    HANDLE hProc = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_NO_PROMPTS);

    if (!SymInitialize(hProc, OBFUSCATE_STRING(PSI_SYM_SEARCH_PATH), FALSE)) {
        return FALSE;
    }

    char mod_path[520];
    psi_us_to_ansi(&mod->FullDllName, mod_path, sizeof(mod_path));

    DWORD64 dll_base   = (DWORD64)(uintptr_t)mod->DllBase;
    DWORD   image_size = mod->SizeOfImage;

    DWORD64 loaded = SymLoadModuleEx(hProc, NULL, mod_path, NULL,
                                     dll_base, image_size, NULL, 0);
    if (loaded == 0 && GetLastError() != ERROR_SUCCESS) {
        SymCleanup(hProc);
        return FALSE;
    }

    BOOL ok = FALSE;

    // SYMBOL_INFO is variable-length (Name is a flex array). Heap-allocate —
    // MAX_SYM_NAME is 2000 chars, too big for the BOF stack under __chkstk.
    SIZE_T sym_sz = sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR);
    PSYMBOL_INFO sym = (PSYMBOL_INFO)LocalAlloc(0x0040 /* LPTR */, sym_sz);
    if (sym) {
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen   = MAX_SYM_NAME;

        DWORD64 disp = 0;
        if (SymFromAddr(hProc, (DWORD64)addr, &disp, sym) && sym->NameLen > 0) {
            size_t i = 0;
            while (i < name_cap - 1 && i < sym->NameLen && sym->Name[i]) {
                out_name[i] = sym->Name[i];
                i++;
            }
            out_name[i] = 0;
            *out_disp = (unsigned long long)disp;
            ok = TRUE;
        }
        LocalFree(sym);
    }

    if (loaded != 0) SymUnloadModule64(hProc, loaded);
    SymCleanup(hProc);
    return ok;
}

static void psi_basename_only(const char *full, char *out, size_t cap) {
    if (cap == 0) return;
    out[0] = 0;
    if (!full) return;
    const char *slash = full;
    for (const char *p = full; *p; p++) {
        if (*p == '\\' || *p == '/') slash = p + 1;
    }
    size_t i = 0;
    while (i < cap - 1 && slash[i]) { out[i] = slash[i]; i++; }
    out[i] = 0;
}

void handle_meminfo(datap *parser) {
    char *addr_s = BeaconDataExtract(parser, NULL);

    if (!addr_s || !*addr_s) {
        EPRINT("[-] psi meminfo: missing <address>\n");
        return;
    }

    unsigned long long addr = 0;
    if (!psi_parse_hex_u64(addr_s, &addr)) {
        EPRINT("[-] psi meminfo: malformed hex address '%s'\n", addr_s);
        return;
    }

    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T vq = VirtualQuery((LPCVOID)(uintptr_t)addr, &mbi, sizeof(mbi));
    if (vq == 0) {
        EPRINT("[-] psi meminfo: VirtualQuery failed at 0x%llx (GLE=0x%lx)\n",
               addr, GetLastError());
        return;
    }

    PPSI_LDR_DATA_TABLE_ENTRY mod = psi_find_backing_module(addr);

    formatp buf;
    BeaconFormatAlloc(&buf, 8192);

    FPRINT(&buf, "[+] psi meminfo 0x%llx\n\n", addr);
    FPRINT(&buf, "Address         : 0x%016llx\n", addr);

    char mod_base[260];
    char mod_full[520];
    BOOL ran_pe_walk = FALSE;
    BOOL eat_ok      = FALSE;
    char eat_name[256];
    DWORD eat_off = 0;

    if (mod) {
        psi_us_to_ansi(&mod->BaseDllName, mod_base, sizeof(mod_base));
        psi_us_to_ansi(&mod->FullDllName, mod_full, sizeof(mod_full));

        FPRINT(&buf, "Module          : %s\n", mod_base);
        FPRINT(&buf, "    FullDllName : %s\n", mod_full);
        FPRINT(&buf, "    DllBase     : 0x%016llx\n",
               (unsigned long long)(uintptr_t)mod->DllBase);
        FPRINT(&buf, "    SizeOfImage : 0x%lx\n", (unsigned long)mod->SizeOfImage);

        DWORD rva = (DWORD)(addr - (unsigned long long)(uintptr_t)mod->DllBase);
        FPRINT(&buf, "    RVA         : 0x%08lx\n", (unsigned long)rva);

        if (mbi.Type == MEM_IMAGE) {
            ran_pe_walk = TRUE;
            IMAGE_NT_HEADERS64 *nt = psi_nt_headers(mod->DllBase);
            if (!nt) {
                FPRINT(&buf, "    Section     : <headers unreadable>\n");
            } else {
                char  sec_name[16] = {0};
                DWORD sec_start = 0, sec_end = 0, sec_char = 0;
                BOOL  in_hdrs = FALSE;
                if (psi_section_for_rva(nt, rva, sec_name,
                                        &sec_start, &sec_end, &sec_char,
                                        &in_hdrs)) {
                    FPRINT(&buf, "    Section     : %s  (0x%08lx..0x%08lx, ",
                           sec_name,
                           (unsigned long)sec_start, (unsigned long)sec_end);
                    if (in_hdrs) FPRINT(&buf, "R--");
                    else         psi_print_section_perm(&buf, sec_char);
                    FPRINT(&buf, ")\n");
                } else {
                    FPRINT(&buf, "    Section     : <not found>\n");
                }

                eat_ok = psi_nearest_eat_export(
                    mod->DllBase, mod->SizeOfImage, rva, eat_name, &eat_off);
            }
        } else {
            FPRINT(&buf, "    Section     : <module not mapped as image>\n");
        }
    } else {
        FPRINT(&buf, "Module          : <none>\n");
    }

    FPRINT(&buf, "\nSymbols\n");
    if (mod && ran_pe_walk) {
        char eat_mod[260];
        psi_basename_only(mod_base, eat_mod, sizeof(eat_mod));

        char sym_name[512];
        unsigned long long sym_disp = 0;
        BOOL sym_ok = psi_sym_lookup(addr, mod, sym_name, sizeof(sym_name),
                                     &sym_disp);

        BOOL agree = eat_ok && sym_ok
                  && psi_streq(eat_name, sym_name)
                  && (unsigned long long)eat_off == sym_disp;

        if (agree) {
            FPRINT(&buf, "    Nearest     : %s!%s+0x%lx\n",
                   eat_mod, eat_name, (unsigned long)eat_off);
        } else {
            if (eat_ok) {
                FPRINT(&buf, "    EAT         : %s!%s+0x%lx\n",
                       eat_mod, eat_name, (unsigned long)eat_off);
            } else {
                FPRINT(&buf, "    EAT         : <none>\n");
            }
            if (sym_ok) {
                FPRINT(&buf, "    symsrv      : %s!%s+0x%llx\n",
                       eat_mod, sym_name, sym_disp);
            } else {
                FPRINT(&buf, "    symsrv      : <unavailable>\n");
            }
        }
    } else {
        FPRINT(&buf, "    Nearest     : <no backing image>\n");
    }

    FPRINT(&buf, "\nRegion\n");
    FPRINT(&buf, "    BaseAddress     : 0x%016llx\n",
           (unsigned long long)(uintptr_t)mbi.BaseAddress);
    FPRINT(&buf, "    AllocationBase  : 0x%016llx\n",
           (unsigned long long)(uintptr_t)mbi.AllocationBase);
    FPRINT(&buf, "    AllocationProt  : ");
    psi_print_protect(&buf, mbi.AllocationProtect);
    FPRINT(&buf, "\n    RegionSize      : 0x%llx (%llu bytes)\n",
           (unsigned long long)mbi.RegionSize,
           (unsigned long long)mbi.RegionSize);
    FPRINT(&buf, "    State           : %s\n", psi_mem_state_str(mbi.State));
    FPRINT(&buf, "    Protect         : ");
    psi_print_protect(&buf, mbi.Protect);
    FPRINT(&buf, "\n    Type            : %s", psi_mem_type_str(mbi.Type));

    FLUSHFREE(&buf);
}
