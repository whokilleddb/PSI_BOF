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
