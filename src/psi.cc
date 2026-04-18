#include "psi.h"

// clang's loop-idiom pass folds byte loops into calls to strlen / memcpy /
// memset, which Beacon's loader cannot resolve. Provide local implementations
// and tag them optnone so the compiler doesn't recursively rewrite their
// bodies into calls to themselves.
extern "C" {
    __attribute__((optnone))
    size_t strlen(const char *s) {
        const char *p = s;
        while (*p) ++p;
        return (size_t)(p - s);
    }

    __attribute__((optnone))
    void *memcpy(void *dst, const void *src, size_t n) {
        unsigned char       *d = (unsigned char *)dst;
        const unsigned char *s = (const unsigned char *)src;
        while (n--) *d++ = *s++;
        return dst;
    }

    __attribute__((optnone))
    void *memset(void *dst, int v, size_t n) {
        unsigned char *d = (unsigned char *)dst;
        while (n--) *d++ = (unsigned char)v;
        return dst;
    }
}

static size_t psi_strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }

static int psi_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

static char    psi_aupper(char c)    { return (c >= 'a'  && c <= 'z' ) ? (char)(c - 32)    : c; }
static wchar_t psi_wupper(wchar_t c) { return (c >= L'a' && c <= L'z') ? (wchar_t)(c - 32) : c; }

static int psi_ieq_wa(const wchar_t *w, size_t wlen, const char *a, size_t alen) {
    if (wlen != alen) return 0;
    for (size_t i = 0; i < wlen; i++) {
        if (psi_wupper(w[i]) != (wchar_t)psi_aupper(a[i])) return 0;
    }
    return 1;
}

static int psi_basename_matches(const UNICODE_STRING *u, const char *name) {
    if (!u->Buffer || !name) return 0;
    size_t wlen = u->Length / sizeof(wchar_t);
    size_t alen = psi_strlen(name);

    if (psi_ieq_wa(u->Buffer, wlen, name, alen)) return 1;

    // Retry with ".dll" appended so `psi ba ntdll` matches `ntdll.dll`.
    char buf[512];
    if (alen + 4 >= sizeof(buf)) return 0;
    for (size_t i = 0; i < alen; i++) buf[i] = name[i];
    buf[alen]     = '.';
    buf[alen + 1] = 'd';
    buf[alen + 2] = 'l';
    buf[alen + 3] = 'l';
    return psi_ieq_wa(u->Buffer, wlen, buf, alen + 4);
}

static void psi_us_to_ansi(const UNICODE_STRING *u, char *out, size_t cap) {
    if (cap == 0) return;
    if (!u->Buffer) { out[0] = 0; return; }
    size_t wlen = u->Length / sizeof(wchar_t);
    size_t n    = wlen < (cap - 1) ? wlen : (cap - 1);
    for (size_t i = 0; i < n; i++) {
        wchar_t c = u->Buffer[i];
        out[i] = (c < 0x80) ? (char)c : '?';
    }
    out[n] = 0;
}

static void handle_ba(datap *parser) {
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

static void handle_addr(datap *parser) {
    UNUSED(parser);
    EPRINT("[-] psi addr: to be implemented\n");
}

static void handle_cmdline(const char *subcmd, datap *parser) {
    if (!subcmd) {
        EPRINT("[-] psi: missing subcommand\n");
        return;
    }

    if      (psi_streq(subcmd, "ba"))   handle_ba(parser);
    else if (psi_streq(subcmd, "addr")) handle_addr(parser);
    else    EPRINT("[-] psi: unknown subcommand '%s'\n", subcmd);
}

extern "C" void go(char *args, int length) {
    datap parser;
    BeaconDataParse(&parser, args, length);

    char *subcmd = BeaconDataExtract(&parser, NULL);
    handle_cmdline(subcmd, &parser);
}
