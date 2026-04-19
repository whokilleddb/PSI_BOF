#include "psi.h"

void handle_ba(datap *parser) {
    char *dll = BeaconDataExtract(parser, NULL);
    if (!dll || !*dll) {
        EPRINT("[-] psi ba: empty DLL name\n");
        return;
    }

    PPEB peb = (PPEB)__readgsqword(0x60);
    if (!peb || !peb->Ldr) {
        EPRINT("[-] psi ba: PEB/Ldr unavailable\n");
        return;
    }

    PLIST_ENTRY head = &peb->Ldr->InMemoryOrderModuleList;
    for (PLIST_ENTRY cur = head->Flink; cur && cur != head; cur = cur->Flink) {
        PPSI_LDR_DATA_TABLE_ENTRY e =
            CONTAINING_RECORD(cur, PSI_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!psi_basename_matches(&e->BaseDllName, dll)) continue;

        char base[260];
        char full[520];
        psi_us_to_ansi(&e->BaseDllName, base, sizeof(base));
        psi_us_to_ansi(&e->FullDllName, full, sizeof(full));

        formatp buf;
        BeaconFormatAlloc(&buf, 1024);
        FPRINT(&buf, "[+] %s\n",                          base);
        FPRINT(&buf, "    FullDllName : %s\n",            full);
        FPRINT(&buf, "    DllBase     : 0x%llx\n",        (unsigned long long)e->DllBase);
        FPRINT(&buf, "    EntryPoint  : 0x%llx\n",        (unsigned long long)e->EntryPoint);
        FPRINT(&buf, "    SizeOfImage : 0x%lx (%lu bytes)", e->SizeOfImage, e->SizeOfImage);
        FLUSHFREE(&buf);
        return;
    }

    EPRINT("[-] psi ba: module '%s' not found in current process\n", dll);
}
