#include "psi.h"

// Read a little-endian unsigned value of `size` bytes (1..8) from p.
static unsigned long long psi_load_le(const unsigned char *p, size_t size) {
    unsigned long long v = 0;
    for (size_t i = 0; i < size; i++) {
        v |= ((unsigned long long)p[i]) << (i * 8);
    }
    return v;
}

// 16-byte-per-row grouped hexdump. `chunk` must be in {1, 2, 4, 8} — divides 16.
// Each chunk prints as "0x<little-endian value>", e.g. for WORD: "0xaabb 0xccdd".
static void psi_hexdump(formatp *fb, unsigned long long base,
                        const unsigned char *data, size_t total, size_t chunk) {
    const size_t row_bytes      = 16;
    const size_t chunks_per_row = row_bytes / chunk;
    const size_t chunk_width    = chunk * 2 + 3;   // "0x" + hex digits + trailing space
    const size_t row_hex_width  = chunks_per_row * chunk_width;

    size_t off = 0;
    while (off < total) {
        size_t remaining = total - off;
        size_t row_len   = remaining < row_bytes ? remaining : row_bytes;

        FPRINT(fb, "0x%016llx  ", base + off);

        size_t written   = 0;
        size_t chunk_off = 0;
        while (chunk_off < row_len) {
            size_t take = (row_len - chunk_off) >= chunk ? chunk : (row_len - chunk_off);
            unsigned long long v = psi_load_le(data + off + chunk_off, take);
            FPRINT(fb, "0x%0*llx ", (int)(take * 2), v);
            written   += take * 2 + 3;
            chunk_off += take;
        }
        for (size_t i = written; i < row_hex_width; i++) FPRINT(fb, " ");

        FPRINT(fb, " ");
        for (size_t i = 0; i < row_len; i++) {
            unsigned char c = data[off + i];
            FPRINT(fb, "%c", (c >= 0x20 && c <= 0x7e) ? (char)c : '.');
        }
        FPRINT(fb, "\n");

        off += row_len;
    }
}

void handle_addr(datap *parser) {
    char *addr_s = BeaconDataExtract(parser, NULL);
    char *type_s = BeaconDataExtract(parser, NULL);
    int   count  = BeaconDataInt(parser);

    if (!addr_s || !*addr_s) {
        EPRINT("[-] psi addr: missing <address>\n");
        return;
    }
    if (!type_s || !*type_s) {
        EPRINT("[-] psi addr: missing <type>\n");
        return;
    }
    if (count <= 0) {
        EPRINT("[-] psi addr: count must be > 0 (got %d)\n", count);
        return;
    }

    unsigned long long addr = 0;
    if (!psi_parse_hex_u64(addr_s, &addr)) {
        EPRINT("[-] psi addr: malformed hex address '%s'\n", addr_s);
        return;
    }

    size_t tsize = psi_type_size(type_s);
    if (tsize == 0) {
        EPRINT("[-] psi addr: unknown type '%s', expected one of: "
               "BYTE, WORD, DWORD, LPVOID\n", type_s);
        return;
    }

    size_t total = (size_t)count * tsize;
    if (total / tsize != (size_t)count) {
        EPRINT("[-] psi addr: count * sizeof(%s) overflows\n", type_s);
        return;
    }
    if (addr + (unsigned long long)total < addr) {
        EPRINT("[-] psi addr: address range wraps past 0 (0x%llx + 0x%llx)\n",
               addr, (unsigned long long)total);
        return;
    }

    // Heap-allocate the read buffer. A large stack array would trip __chkstk,
    // which BOFs can't resolve.
    unsigned char *rb = (unsigned char *)LocalAlloc(0x0040 /* LPTR */, total);
    if (!rb) {
        EPRINT("[-] psi addr: LocalAlloc(%llu) failed (GLE=0x%lx)\n",
               (unsigned long long)total, GetLastError());
        return;
    }

    SIZE_T nread = 0;
    BOOL ok = ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(uintptr_t)addr,
                                rb, total, &nread);
    if (!ok && nread == 0) {
        EPRINT("[-] psi addr: ReadProcessMemory failed at 0x%llx (GLE=0x%lx)\n",
               addr, GetLastError());
        LocalFree(rb);
        return;
    }

    formatp buf;
    BeaconFormatAlloc(&buf, 65536);

    FPRINT(&buf, "[+] psi addr 0x%llx %s %d\n", addr, type_s, count);
    psi_hexdump(&buf, addr, rb, (size_t)nread, tsize);
    if ((size_t)nread < total) {
        FPRINT(&buf, "[!] partial read: %llu / %llu bytes (GLE=0x%lx)\n",
               (unsigned long long)nread, (unsigned long long)total, GetLastError());
    }
    FPRINT(&buf, "\n[+] read %llu bytes @ 0x%llx",
           (unsigned long long)nread, addr);
    FLUSHFREE(&buf);

    LocalFree(rb);
}
