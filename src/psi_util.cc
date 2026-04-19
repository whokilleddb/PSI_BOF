#include "psi.h"

static size_t psi_strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }

int psi_streq(const char *a, const char *b) {
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

int psi_basename_matches(const UNICODE_STRING *u, const char *name) {
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

void psi_us_to_ansi(const UNICODE_STRING *u, char *out, size_t cap) {
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

static int psi_aieq(const char *a, const char *b) {
    while (*a && *b) {
        if (psi_aupper(*a) != psi_aupper(*b)) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

int psi_parse_hex_u64(const char *s, unsigned long long *out) {
    if (!s || !*s || !out) return 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    if (!*s) return 0;

    unsigned long long v = 0;
    while (*s) {
        unsigned long long d;
        char c = *s;
        if      (c >= '0' && c <= '9') d = (unsigned long long)(c - '0');
        else if (c >= 'a' && c <= 'f') d = (unsigned long long)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = (unsigned long long)(c - 'A' + 10);
        else return 0;

        if (v > (~0ULL >> 4)) return 0;
        v = (v << 4) | d;
        s++;
    }
    *out = v;
    return 1;
}

size_t psi_type_size(const char *t) {
    if (!t) return 0;
    if (psi_aieq(t, "BYTE"))    return 1;
    if (psi_aieq(t, "WORD"))    return 2;
    if (psi_aieq(t, "DWORD"))   return 4;
    if (psi_aieq(t, "LPVOID"))  return 8;
    return 0;
}

const char *psi_mem_state_str(DWORD state) {
    switch (state) {
        case MEM_COMMIT:  return "MEM_COMMIT";
        case MEM_RESERVE: return "MEM_RESERVE";
        case MEM_FREE:    return "MEM_FREE";
        default:          return "UNKNOWN";
    }
}

const char *psi_mem_type_str(DWORD type) {
    switch (type) {
        case MEM_IMAGE:   return "MEM_IMAGE";
        case MEM_MAPPED:  return "MEM_MAPPED";
        case MEM_PRIVATE: return "MEM_PRIVATE";
        case 0:           return "-";
        default:          return "UNKNOWN";
    }
}

void psi_print_protect(formatp *fb, DWORD prot) {
    DWORD base = prot & ~(PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE);
    const char *name = "UNKNOWN";
    const char *rwx  = "???";
    switch (base) {
        case 0:                      name = "(none)";                 rwx = "---"; break;
        case PAGE_NOACCESS:          name = "PAGE_NOACCESS";          rwx = "---"; break;
        case PAGE_READONLY:          name = "PAGE_READONLY";          rwx = "R--"; break;
        case PAGE_READWRITE:         name = "PAGE_READWRITE";         rwx = "RW-"; break;
        case PAGE_WRITECOPY:         name = "PAGE_WRITECOPY";         rwx = "RW-"; break;
        case PAGE_EXECUTE:           name = "PAGE_EXECUTE";           rwx = "--X"; break;
        case PAGE_EXECUTE_READ:      name = "PAGE_EXECUTE_READ";      rwx = "R-X"; break;
        case PAGE_EXECUTE_READWRITE: name = "PAGE_EXECUTE_READWRITE"; rwx = "RWX"; break;
        case PAGE_EXECUTE_WRITECOPY: name = "PAGE_EXECUTE_WRITECOPY"; rwx = "RWX"; break;
    }
    FPRINT(fb, "%s", name);
    if (prot & PAGE_GUARD)        FPRINT(fb, "|PAGE_GUARD");
    if (prot & PAGE_NOCACHE)      FPRINT(fb, "|PAGE_NOCACHE");
    if (prot & PAGE_WRITECOMBINE) FPRINT(fb, "|PAGE_WRITECOMBINE");
    FPRINT(fb, " (%s)", rwx);
}
