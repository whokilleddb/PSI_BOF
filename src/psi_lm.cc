#include "psi.h"

static void psi_print_module(formatp *buf, PPSI_LDR_DATA_TABLE_ENTRY e) {
    char base[260];
    char full[520];
    psi_us_to_ansi(&e->BaseDllName, base, sizeof(base));
    psi_us_to_ansi(&e->FullDllName, full, sizeof(full));

    FPRINT(buf, "[+] %s\n",                            base);
    FPRINT(buf, "    FullDllName : %s\n",              full);
    FPRINT(buf, "    DllBase     : 0x%llx\n",          (unsigned long long)e->DllBase);
    FPRINT(buf, "    EntryPoint  : 0x%llx\n",          (unsigned long long)e->EntryPoint);
    FPRINT(buf, "    SizeOfImage : 0x%lx (%lu bytes)\n", e->SizeOfImage, e->SizeOfImage);
}

void handle_lm(datap *parser) {
    char *dll = BeaconDataExtract(parser, NULL);

    PPEB peb = (PPEB)__readgsqword(0x60);
    if (!peb || !peb->Ldr) {
        EPRINT("[-] psi lm: PEB/Ldr unavailable\n");
        return;
    }

    PLIST_ENTRY head = &peb->Ldr->InMemoryOrderModuleList;

    if (dll && *dll) {
        for (PLIST_ENTRY cur = head->Flink; cur && cur != head; cur = cur->Flink) {
            PPSI_LDR_DATA_TABLE_ENTRY e =
                CONTAINING_RECORD(cur, PSI_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

            if (!psi_basename_matches(&e->BaseDllName, dll)) continue;

            formatp buf;
            BeaconFormatAlloc(&buf, 1024);
            psi_print_module(&buf, e);
            FLUSHFREE(&buf);
            return;
        }

        EPRINT("[-] psi lm: module '%s' not found in current process\n", dll);
        return;
    }

    formatp buf;
    BeaconFormatAlloc(&buf, 65536);

    unsigned long count = 0;
    for (PLIST_ENTRY cur = head->Flink; cur && cur != head; cur = cur->Flink) {
        PPSI_LDR_DATA_TABLE_ENTRY e =
            CONTAINING_RECORD(cur, PSI_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        psi_print_module(&buf, e);
        count++;
    }

    FPRINT(&buf, "[+] Total %lu modules loaded in the current process", count);
    FLUSHFREE(&buf);
}
