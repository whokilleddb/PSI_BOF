#include "psi.h"

// winnt.h defines CONTEXT_XSTATE on Windows 7 SP1+; guard for older SDKs.
#ifndef CONTEXT_XSTATE
#define CONTEXT_XSTATE (CONTEXT_AMD64 | 0x00000040L)
#endif

// XSTATE component IDs, from the Intel SDM / winnt.h.
#define PSI_XSTATE_AVX          2
#define PSI_XSTATE_AVX512_KMASK 5
#define PSI_XSTATE_AVX512_ZMM_H 6
#define PSI_XSTATE_AVX512_ZMM   7

#define PSI_XSTATE_MASK_AVX    (1ULL << PSI_XSTATE_AVX)
#define PSI_XSTATE_MASK_AVX512 ((1ULL << PSI_XSTATE_AVX512_KMASK) | \
                                (1ULL << PSI_XSTATE_AVX512_ZMM_H) | \
                                (1ULL << PSI_XSTATE_AVX512_ZMM))

typedef enum {
    PSI_RC_GPR = 0,
    PSI_RC_FLAGS,
    PSI_RC_SEG,
    PSI_RC_DR,
    PSI_RC_XMM,
    PSI_RC_YMM,
    PSI_RC_ZMM,
    PSI_RC_KMASK,
    PSI_RC_MXCSR,
    PSI_RC_FPCTRL,
    PSI_RC_FPSTAT,
    PSI_RC_FPTAG,
    PSI_RC_FPOP,
    PSI_RC_ST
} psi_reg_class_t;

enum {
    G_RAX = 0, G_RBX, G_RCX, G_RDX, G_RSI, G_RDI, G_RBP, G_RSP,
    G_R8,  G_R9,  G_R10, G_R11, G_R12, G_R13, G_R14, G_R15, G_RIP
};

typedef struct {
    const char   *name;
    unsigned char klass;
    unsigned char index;
    unsigned char width_bits;
    unsigned char byte_shift; // bits (n*8) into the raw 64-bit GPR — used for ah/bh/ch/dh
} psi_reg_alias_t;

static const psi_reg_alias_t psi_reg_table[] = {
    {"rax", PSI_RC_GPR, G_RAX, 64, 0}, {"eax", PSI_RC_GPR, G_RAX, 32, 0},
    {"ax",  PSI_RC_GPR, G_RAX, 16, 0}, {"ah",  PSI_RC_GPR, G_RAX,  8, 1},
    {"al",  PSI_RC_GPR, G_RAX,  8, 0},

    {"rbx", PSI_RC_GPR, G_RBX, 64, 0}, {"ebx", PSI_RC_GPR, G_RBX, 32, 0},
    {"bx",  PSI_RC_GPR, G_RBX, 16, 0}, {"bh",  PSI_RC_GPR, G_RBX,  8, 1},
    {"bl",  PSI_RC_GPR, G_RBX,  8, 0},

    {"rcx", PSI_RC_GPR, G_RCX, 64, 0}, {"ecx", PSI_RC_GPR, G_RCX, 32, 0},
    {"cx",  PSI_RC_GPR, G_RCX, 16, 0}, {"ch",  PSI_RC_GPR, G_RCX,  8, 1},
    {"cl",  PSI_RC_GPR, G_RCX,  8, 0},

    {"rdx", PSI_RC_GPR, G_RDX, 64, 0}, {"edx", PSI_RC_GPR, G_RDX, 32, 0},
    {"dx",  PSI_RC_GPR, G_RDX, 16, 0}, {"dh",  PSI_RC_GPR, G_RDX,  8, 1},
    {"dl",  PSI_RC_GPR, G_RDX,  8, 0},

    {"rsi", PSI_RC_GPR, G_RSI, 64, 0}, {"esi", PSI_RC_GPR, G_RSI, 32, 0},
    {"si",  PSI_RC_GPR, G_RSI, 16, 0}, {"sil", PSI_RC_GPR, G_RSI,  8, 0},

    {"rdi", PSI_RC_GPR, G_RDI, 64, 0}, {"edi", PSI_RC_GPR, G_RDI, 32, 0},
    {"di",  PSI_RC_GPR, G_RDI, 16, 0}, {"dil", PSI_RC_GPR, G_RDI,  8, 0},

    {"rbp", PSI_RC_GPR, G_RBP, 64, 0}, {"ebp", PSI_RC_GPR, G_RBP, 32, 0},
    {"bp",  PSI_RC_GPR, G_RBP, 16, 0}, {"bpl", PSI_RC_GPR, G_RBP,  8, 0},

    {"rsp", PSI_RC_GPR, G_RSP, 64, 0}, {"esp", PSI_RC_GPR, G_RSP, 32, 0},
    {"sp",  PSI_RC_GPR, G_RSP, 16, 0}, {"spl", PSI_RC_GPR, G_RSP,  8, 0},

    {"r8",   PSI_RC_GPR, G_R8,  64, 0}, {"r8d",  PSI_RC_GPR, G_R8,  32, 0},
    {"r8w",  PSI_RC_GPR, G_R8,  16, 0}, {"r8b",  PSI_RC_GPR, G_R8,   8, 0},
    {"r9",   PSI_RC_GPR, G_R9,  64, 0}, {"r9d",  PSI_RC_GPR, G_R9,  32, 0},
    {"r9w",  PSI_RC_GPR, G_R9,  16, 0}, {"r9b",  PSI_RC_GPR, G_R9,   8, 0},
    {"r10",  PSI_RC_GPR, G_R10, 64, 0}, {"r10d", PSI_RC_GPR, G_R10, 32, 0},
    {"r10w", PSI_RC_GPR, G_R10, 16, 0}, {"r10b", PSI_RC_GPR, G_R10,  8, 0},
    {"r11",  PSI_RC_GPR, G_R11, 64, 0}, {"r11d", PSI_RC_GPR, G_R11, 32, 0},
    {"r11w", PSI_RC_GPR, G_R11, 16, 0}, {"r11b", PSI_RC_GPR, G_R11,  8, 0},
    {"r12",  PSI_RC_GPR, G_R12, 64, 0}, {"r12d", PSI_RC_GPR, G_R12, 32, 0},
    {"r12w", PSI_RC_GPR, G_R12, 16, 0}, {"r12b", PSI_RC_GPR, G_R12,  8, 0},
    {"r13",  PSI_RC_GPR, G_R13, 64, 0}, {"r13d", PSI_RC_GPR, G_R13, 32, 0},
    {"r13w", PSI_RC_GPR, G_R13, 16, 0}, {"r13b", PSI_RC_GPR, G_R13,  8, 0},
    {"r14",  PSI_RC_GPR, G_R14, 64, 0}, {"r14d", PSI_RC_GPR, G_R14, 32, 0},
    {"r14w", PSI_RC_GPR, G_R14, 16, 0}, {"r14b", PSI_RC_GPR, G_R14,  8, 0},
    {"r15",  PSI_RC_GPR, G_R15, 64, 0}, {"r15d", PSI_RC_GPR, G_R15, 32, 0},
    {"r15w", PSI_RC_GPR, G_R15, 16, 0}, {"r15b", PSI_RC_GPR, G_R15,  8, 0},

    {"rip", PSI_RC_GPR, G_RIP, 64, 0}, {"eip", PSI_RC_GPR, G_RIP, 32, 0},
    {"ip",  PSI_RC_GPR, G_RIP, 16, 0},

    {"rflags", PSI_RC_FLAGS, 0, 64, 0}, {"eflags", PSI_RC_FLAGS, 0, 32, 0},
    {"flags",  PSI_RC_FLAGS, 0, 16, 0},

    {"cs", PSI_RC_SEG, 0, 16, 0}, {"ds", PSI_RC_SEG, 1, 16, 0},
    {"es", PSI_RC_SEG, 2, 16, 0}, {"fs", PSI_RC_SEG, 3, 16, 0},
    {"gs", PSI_RC_SEG, 4, 16, 0}, {"ss", PSI_RC_SEG, 5, 16, 0},

    {"dr0", PSI_RC_DR, 0, 64, 0}, {"dr1", PSI_RC_DR, 1, 64, 0},
    {"dr2", PSI_RC_DR, 2, 64, 0}, {"dr3", PSI_RC_DR, 3, 64, 0},
    {"dr6", PSI_RC_DR, 6, 64, 0}, {"dr7", PSI_RC_DR, 7, 64, 0},

    {"mxcsr", PSI_RC_MXCSR,  0, 32, 0},
    {"fctrl", PSI_RC_FPCTRL, 0, 16, 0},
    {"fstat", PSI_RC_FPSTAT, 0, 16, 0},
    {"ftag",  PSI_RC_FPTAG,  0,  8, 0},
    {"fop",   PSI_RC_FPOP,   0, 16, 0},

    {"st0", PSI_RC_ST, 0, 80, 0}, {"st1", PSI_RC_ST, 1, 80, 0},
    {"st2", PSI_RC_ST, 2, 80, 0}, {"st3", PSI_RC_ST, 3, 80, 0},
    {"st4", PSI_RC_ST, 4, 80, 0}, {"st5", PSI_RC_ST, 5, 80, 0},
    {"st6", PSI_RC_ST, 6, 80, 0}, {"st7", PSI_RC_ST, 7, 80, 0},

    {"xmm0",  PSI_RC_XMM,  0, 128, 0}, {"xmm1",  PSI_RC_XMM,  1, 128, 0},
    {"xmm2",  PSI_RC_XMM,  2, 128, 0}, {"xmm3",  PSI_RC_XMM,  3, 128, 0},
    {"xmm4",  PSI_RC_XMM,  4, 128, 0}, {"xmm5",  PSI_RC_XMM,  5, 128, 0},
    {"xmm6",  PSI_RC_XMM,  6, 128, 0}, {"xmm7",  PSI_RC_XMM,  7, 128, 0},
    {"xmm8",  PSI_RC_XMM,  8, 128, 0}, {"xmm9",  PSI_RC_XMM,  9, 128, 0},
    {"xmm10", PSI_RC_XMM, 10, 128, 0}, {"xmm11", PSI_RC_XMM, 11, 128, 0},
    {"xmm12", PSI_RC_XMM, 12, 128, 0}, {"xmm13", PSI_RC_XMM, 13, 128, 0},
    {"xmm14", PSI_RC_XMM, 14, 128, 0}, {"xmm15", PSI_RC_XMM, 15, 128, 0},

    {"ymm0",  PSI_RC_YMM,  0, 0, 0}, {"ymm1",  PSI_RC_YMM,  1, 0, 0},
    {"ymm2",  PSI_RC_YMM,  2, 0, 0}, {"ymm3",  PSI_RC_YMM,  3, 0, 0},
    {"ymm4",  PSI_RC_YMM,  4, 0, 0}, {"ymm5",  PSI_RC_YMM,  5, 0, 0},
    {"ymm6",  PSI_RC_YMM,  6, 0, 0}, {"ymm7",  PSI_RC_YMM,  7, 0, 0},
    {"ymm8",  PSI_RC_YMM,  8, 0, 0}, {"ymm9",  PSI_RC_YMM,  9, 0, 0},
    {"ymm10", PSI_RC_YMM, 10, 0, 0}, {"ymm11", PSI_RC_YMM, 11, 0, 0},
    {"ymm12", PSI_RC_YMM, 12, 0, 0}, {"ymm13", PSI_RC_YMM, 13, 0, 0},
    {"ymm14", PSI_RC_YMM, 14, 0, 0}, {"ymm15", PSI_RC_YMM, 15, 0, 0},

    {"zmm0",  PSI_RC_ZMM,  0, 0, 0}, {"zmm1",  PSI_RC_ZMM,  1, 0, 0},
    {"zmm2",  PSI_RC_ZMM,  2, 0, 0}, {"zmm3",  PSI_RC_ZMM,  3, 0, 0},
    {"zmm4",  PSI_RC_ZMM,  4, 0, 0}, {"zmm5",  PSI_RC_ZMM,  5, 0, 0},
    {"zmm6",  PSI_RC_ZMM,  6, 0, 0}, {"zmm7",  PSI_RC_ZMM,  7, 0, 0},
    {"zmm8",  PSI_RC_ZMM,  8, 0, 0}, {"zmm9",  PSI_RC_ZMM,  9, 0, 0},
    {"zmm10", PSI_RC_ZMM, 10, 0, 0}, {"zmm11", PSI_RC_ZMM, 11, 0, 0},
    {"zmm12", PSI_RC_ZMM, 12, 0, 0}, {"zmm13", PSI_RC_ZMM, 13, 0, 0},
    {"zmm14", PSI_RC_ZMM, 14, 0, 0}, {"zmm15", PSI_RC_ZMM, 15, 0, 0},
    {"zmm16", PSI_RC_ZMM, 16, 0, 0}, {"zmm17", PSI_RC_ZMM, 17, 0, 0},
    {"zmm18", PSI_RC_ZMM, 18, 0, 0}, {"zmm19", PSI_RC_ZMM, 19, 0, 0},
    {"zmm20", PSI_RC_ZMM, 20, 0, 0}, {"zmm21", PSI_RC_ZMM, 21, 0, 0},
    {"zmm22", PSI_RC_ZMM, 22, 0, 0}, {"zmm23", PSI_RC_ZMM, 23, 0, 0},
    {"zmm24", PSI_RC_ZMM, 24, 0, 0}, {"zmm25", PSI_RC_ZMM, 25, 0, 0},
    {"zmm26", PSI_RC_ZMM, 26, 0, 0}, {"zmm27", PSI_RC_ZMM, 27, 0, 0},
    {"zmm28", PSI_RC_ZMM, 28, 0, 0}, {"zmm29", PSI_RC_ZMM, 29, 0, 0},
    {"zmm30", PSI_RC_ZMM, 30, 0, 0}, {"zmm31", PSI_RC_ZMM, 31, 0, 0},

    {"k0", PSI_RC_KMASK, 0, 64, 0}, {"k1", PSI_RC_KMASK, 1, 64, 0},
    {"k2", PSI_RC_KMASK, 2, 64, 0}, {"k3", PSI_RC_KMASK, 3, 64, 0},
    {"k4", PSI_RC_KMASK, 4, 64, 0}, {"k5", PSI_RC_KMASK, 5, 64, 0},
    {"k6", PSI_RC_KMASK, 6, 64, 0}, {"k7", PSI_RC_KMASK, 7, 64, 0},
};

static const unsigned psi_reg_table_len =
    sizeof(psi_reg_table) / sizeof(psi_reg_table[0]);

static char psi_regd_upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }

static int psi_regd_ieq(const char *a, const char *b) {
    while (*a && *b) {
        if (psi_regd_upper(*a) != psi_regd_upper(*b)) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

static const psi_reg_alias_t *psi_find_alias(const char *name) {
    for (unsigned i = 0; i < psi_reg_table_len; i++) {
        if (psi_regd_ieq(psi_reg_table[i].name, name)) return &psi_reg_table[i];
    }
    return NULL;
}

static ULONG64 psi_gpr_value(const CONTEXT *ctx, unsigned idx) {
    switch (idx) {
        case G_RAX: return ctx->Rax; case G_RBX: return ctx->Rbx;
        case G_RCX: return ctx->Rcx; case G_RDX: return ctx->Rdx;
        case G_RSI: return ctx->Rsi; case G_RDI: return ctx->Rdi;
        case G_RBP: return ctx->Rbp; case G_RSP: return ctx->Rsp;
        case G_R8:  return ctx->R8;  case G_R9:  return ctx->R9;
        case G_R10: return ctx->R10; case G_R11: return ctx->R11;
        case G_R12: return ctx->R12; case G_R13: return ctx->R13;
        case G_R14: return ctx->R14; case G_R15: return ctx->R15;
        case G_RIP: return ctx->Rip;
    }
    return 0;
}

static WORD psi_seg_value(const CONTEXT *ctx, unsigned idx) {
    switch (idx) {
        case 0: return ctx->SegCs; case 1: return ctx->SegDs;
        case 2: return ctx->SegEs; case 3: return ctx->SegFs;
        case 4: return ctx->SegGs; case 5: return ctx->SegSs;
    }
    return 0;
}

static ULONG64 psi_dr_value(const CONTEXT *ctx, unsigned idx) {
    switch (idx) {
        case 0: return ctx->Dr0; case 1: return ctx->Dr1;
        case 2: return ctx->Dr2; case 3: return ctx->Dr3;
        case 6: return ctx->Dr6; case 7: return ctx->Dr7;
    }
    return 0;
}

// Mask/shift a 64-bit GPR into a sub-width view and emit as "0x<hex>".
static void psi_fprint_width(formatp *buf, ULONG64 raw,
                             unsigned width_bits, unsigned byte_shift) {
    ULONG64 v = raw;
    if (byte_shift) v >>= (byte_shift * 8);
    int digits;
    switch (width_bits) {
        case 64: digits = 16;                                     break;
        case 32: v &= 0xffffffffULL;          digits = 8;         break;
        case 16: v &= 0xffffULL;              digits = 4;         break;
        case 8:  v &= 0xffULL;                digits = 2;         break;
        default: digits = 16;                                     break;
    }
    FPRINT(buf, "0x%0*llx", digits, (unsigned long long)v);
}

// Print n bytes high-to-low (byte n-1 first) as hex, with an underscore every
// 16 bytes after the first chunk. Prefixes "0x".
static void psi_fprint_bytes_be(formatp *buf, const unsigned char *bytes, unsigned n) {
    FPRINT(buf, "0x");
    for (unsigned i = 0; i < n; i++) {
        if (i > 0 && (i % 16) == 0) FPRINT(buf, "_");
        FPRINT(buf, "%02x", bytes[n - 1 - i]);
    }
}

static void psi_fprint_flags(formatp *buf, ULONG64 rflags) {
    FPRINT(buf, "    rflags = 0x%016llx  [", (unsigned long long)rflags);
    static const struct { const char *n; int bit; } F[] = {
        {"CF",0},{"PF",2},{"AF",4},{"ZF",6},{"SF",7},
        {"TF",8},{"IF",9},{"DF",10},{"OF",11}
    };
    int first = 1;
    for (unsigned i = 0; i < sizeof(F)/sizeof(F[0]); i++) {
        if (rflags & (1ULL << F[i].bit)) {
            if (!first) FPRINT(buf, " ");
            FPRINT(buf, "%s", F[i].n);
            first = 0;
        }
    }
    FPRINT(buf, "]\n");
}

// ==== XSTATE accessors ==========================================================
// All of these require a CONTEXT_EX allocated by InitializeContext with
// CONTEXT_XSTATE enabled *and* the corresponding feature bit SetXStateFeaturesMask'd
// in. Callers must gate on xstate_avx_available / xstate_avx512_available.

static const unsigned char *psi_locate_xfeat(PCONTEXT ctx, DWORD feat, DWORD *out_len) {
    DWORD len = 0;
    PVOID p = LocateXStateFeature(ctx, feat, &len);
    if (out_len) *out_len = len;
    return (const unsigned char *)p;
}

// Fill 16 bytes (little-endian) for XMMn from ctx->FltSave.XmmRegisters[n].
static void psi_get_xmm_bytes(PCONTEXT ctx, unsigned n, unsigned char out[16]) {
    const M128A *m = &ctx->FltSave.XmmRegisters[n];
    memcpy(out + 0, &m->Low,  8);
    memcpy(out + 8, &m->High, 8);
}

// Fill 32 bytes for YMMn — low 16 from XMM, high 16 from XSTATE_AVX slot n.
static BOOL psi_get_ymm_bytes(PCONTEXT ctx, unsigned n, unsigned char out[32]) {
    psi_get_xmm_bytes(ctx, n, out);
    DWORD len = 0;
    const unsigned char *avx = psi_locate_xfeat(ctx, PSI_XSTATE_AVX, &len);
    if (!avx || len < (n + 1) * 16) {
        memset(out + 16, 0, 16);
        return FALSE;
    }
    memcpy(out + 16, avx + n * 16, 16);
    return TRUE;
}

// Fill 64 bytes for ZMMn.
//   n <  16: low 16 from XMM, next 16 from XSTATE_AVX, upper 32 from XSTATE_AVX512_ZMM_H[n].
//   n >= 16: all 64 bytes from XSTATE_AVX512_ZMM[n-16].
static BOOL psi_get_zmm_bytes(PCONTEXT ctx, unsigned n, unsigned char out[64]) {
    if (n < 16) {
        psi_get_xmm_bytes(ctx, n, out);
        DWORD len = 0;
        const unsigned char *avx = psi_locate_xfeat(ctx, PSI_XSTATE_AVX, &len);
        if (!avx || len < (n + 1) * 16) return FALSE;
        memcpy(out + 16, avx + n * 16, 16);
        const unsigned char *zh = psi_locate_xfeat(ctx, PSI_XSTATE_AVX512_ZMM_H, &len);
        if (!zh || len < (n + 1) * 32) return FALSE;
        memcpy(out + 32, zh + n * 32, 32);
        return TRUE;
    } else {
        DWORD len = 0;
        const unsigned char *zh = psi_locate_xfeat(ctx, PSI_XSTATE_AVX512_ZMM, &len);
        unsigned k = n - 16;
        if (!zh || len < (k + 1) * 64) return FALSE;
        memcpy(out, zh + k * 64, 64);
        return TRUE;
    }
}

static BOOL psi_get_kmask(PCONTEXT ctx, unsigned n, ULONG64 *out) {
    DWORD len = 0;
    const unsigned char *km = psi_locate_xfeat(ctx, PSI_XSTATE_AVX512_KMASK, &len);
    if (!km || len < (n + 1) * 8) return FALSE;
    ULONG64 v = 0;
    memcpy(&v, km + n * 8, 8);
    *out = v;
    return TRUE;
}

// ==== Single-register filter path ===============================================

static void psi_print_single(formatp *buf, PCONTEXT ctx, const psi_reg_alias_t *a,
                             BOOL have_avx, BOOL have_avx512) {
    switch (a->klass) {
        case PSI_RC_GPR: {
            ULONG64 v = psi_gpr_value(ctx, a->index);
            FPRINT(buf, "    %s = ", a->name);
            psi_fprint_width(buf, v, a->width_bits, a->byte_shift);
            FPRINT(buf, "\n");
            break;
        }
        case PSI_RC_FLAGS: {
            ULONG64 v = (ULONG64)ctx->EFlags;
            FPRINT(buf, "    %s = ", a->name);
            psi_fprint_width(buf, v, a->width_bits, 0);
            FPRINT(buf, "\n");
            break;
        }
        case PSI_RC_SEG: {
            FPRINT(buf, "    %s = 0x%04x\n",
                   a->name, (unsigned)psi_seg_value(ctx, a->index));
            break;
        }
        case PSI_RC_DR: {
            FPRINT(buf, "    %s = 0x%016llx\n",
                   a->name, (unsigned long long)psi_dr_value(ctx, a->index));
            break;
        }
        case PSI_RC_MXCSR: {
            FPRINT(buf, "    mxcsr = 0x%08lx\n", (unsigned long)ctx->MxCsr);
            break;
        }
        case PSI_RC_FPCTRL: {
            FPRINT(buf, "    fctrl = 0x%04x\n", (unsigned)ctx->FltSave.ControlWord);
            break;
        }
        case PSI_RC_FPSTAT: {
            FPRINT(buf, "    fstat = 0x%04x\n", (unsigned)ctx->FltSave.StatusWord);
            break;
        }
        case PSI_RC_FPTAG: {
            FPRINT(buf, "    ftag  = 0x%02x\n", (unsigned)ctx->FltSave.TagWord);
            break;
        }
        case PSI_RC_FPOP: {
            FPRINT(buf, "    fop   = 0x%04x\n", (unsigned)ctx->FltSave.ErrorOpcode);
            break;
        }
        case PSI_RC_ST: {
            const M128A *m = &ctx->FltSave.FloatRegisters[a->index];
            unsigned char b[16];
            memcpy(b + 0, &m->Low,  8);
            memcpy(b + 8, &m->High, 8);
            FPRINT(buf, "    st%u = ", (unsigned)a->index);
            // x87 registers are 80-bit; print the first 10 bytes as raw hex low-to-high.
            FPRINT(buf, "0x");
            for (int i = 9; i >= 0; i--) FPRINT(buf, "%02x", b[i]);
            FPRINT(buf, "\n");
            break;
        }
        case PSI_RC_XMM: {
            unsigned char b[16];
            psi_get_xmm_bytes(ctx, a->index, b);
            FPRINT(buf, "    %s = ", a->name);
            psi_fprint_bytes_be(buf, b, 16);
            FPRINT(buf, "\n");
            break;
        }
        case PSI_RC_YMM: {
            if (!have_avx) {
                EPRINT("[-] psi regdump: %s unavailable (CPU/context does not report XSTATE_MASK_AVX)\n", a->name);
                return;
            }
            unsigned char b[32];
            psi_get_ymm_bytes(ctx, a->index, b);
            FPRINT(buf, "    %s = ", a->name);
            psi_fprint_bytes_be(buf, b, 32);
            FPRINT(buf, "\n");
            break;
        }
        case PSI_RC_ZMM: {
            if (!have_avx512) {
                EPRINT("[-] psi regdump: %s unavailable (CPU/context does not report XSTATE_MASK_AVX512)\n", a->name);
                return;
            }
            unsigned char b[64];
            if (!psi_get_zmm_bytes(ctx, a->index, b)) {
                EPRINT("[-] psi regdump: %s unavailable (LocateXStateFeature returned no data)\n", a->name);
                return;
            }
            FPRINT(buf, "    %s = ", a->name);
            psi_fprint_bytes_be(buf, b, 64);
            FPRINT(buf, "\n");
            break;
        }
        case PSI_RC_KMASK: {
            if (!have_avx512) {
                EPRINT("[-] psi regdump: %s unavailable (CPU/context does not report XSTATE_MASK_AVX512)\n", a->name);
                return;
            }
            ULONG64 v = 0;
            if (!psi_get_kmask(ctx, a->index, &v)) {
                EPRINT("[-] psi regdump: %s unavailable (LocateXStateFeature returned no data)\n", a->name);
                return;
            }
            FPRINT(buf, "    %s = 0x%016llx\n", a->name, (unsigned long long)v);
            break;
        }
    }
}

// ==== Full dump path ============================================================

static void psi_print_full(formatp *buf, PCONTEXT ctx,
                           BOOL have_avx, BOOL have_avx512, BOOL self_no_xstate) {
    FPRINT(buf, "General-purpose\n");
    FPRINT(buf, "    rax = 0x%016llx    rbx = 0x%016llx\n",
           (unsigned long long)ctx->Rax, (unsigned long long)ctx->Rbx);
    FPRINT(buf, "    rcx = 0x%016llx    rdx = 0x%016llx\n",
           (unsigned long long)ctx->Rcx, (unsigned long long)ctx->Rdx);
    FPRINT(buf, "    rsi = 0x%016llx    rdi = 0x%016llx\n",
           (unsigned long long)ctx->Rsi, (unsigned long long)ctx->Rdi);
    FPRINT(buf, "    rbp = 0x%016llx    rsp = 0x%016llx\n",
           (unsigned long long)ctx->Rbp, (unsigned long long)ctx->Rsp);
    FPRINT(buf, "    rip = 0x%016llx\n", (unsigned long long)ctx->Rip);
    FPRINT(buf, "    r8  = 0x%016llx    r9  = 0x%016llx\n",
           (unsigned long long)ctx->R8, (unsigned long long)ctx->R9);
    FPRINT(buf, "    r10 = 0x%016llx    r11 = 0x%016llx\n",
           (unsigned long long)ctx->R10, (unsigned long long)ctx->R11);
    FPRINT(buf, "    r12 = 0x%016llx    r13 = 0x%016llx\n",
           (unsigned long long)ctx->R12, (unsigned long long)ctx->R13);
    FPRINT(buf, "    r14 = 0x%016llx    r15 = 0x%016llx\n",
           (unsigned long long)ctx->R14, (unsigned long long)ctx->R15);

    FPRINT(buf, "\nFlags\n");
    psi_fprint_flags(buf, (ULONG64)ctx->EFlags);

    FPRINT(buf, "\nSegment\n");
    FPRINT(buf, "    cs=0x%04x  ds=0x%04x  es=0x%04x  fs=0x%04x  gs=0x%04x  ss=0x%04x\n",
           (unsigned)ctx->SegCs, (unsigned)ctx->SegDs,
           (unsigned)ctx->SegEs, (unsigned)ctx->SegFs,
           (unsigned)ctx->SegGs, (unsigned)ctx->SegSs);

    FPRINT(buf, "\nDebug\n");
    FPRINT(buf, "    dr0=0x%llx  dr1=0x%llx  dr2=0x%llx  dr3=0x%llx  dr6=0x%llx  dr7=0x%llx\n",
           (unsigned long long)ctx->Dr0, (unsigned long long)ctx->Dr1,
           (unsigned long long)ctx->Dr2, (unsigned long long)ctx->Dr3,
           (unsigned long long)ctx->Dr6, (unsigned long long)ctx->Dr7);

    FPRINT(buf, "\nx87 / MXCSR\n");
    FPRINT(buf, "    fctrl=0x%04x  fstat=0x%04x  ftag=0x%02x  fop=0x%04x  mxcsr=0x%08lx\n",
           (unsigned)ctx->FltSave.ControlWord, (unsigned)ctx->FltSave.StatusWord,
           (unsigned)ctx->FltSave.TagWord,     (unsigned)ctx->FltSave.ErrorOpcode,
           (unsigned long)ctx->MxCsr);
    for (unsigned i = 0; i < 8; i++) {
        const M128A *m = &ctx->FltSave.FloatRegisters[i];
        unsigned char b[16];
        memcpy(b + 0, &m->Low,  8);
        memcpy(b + 8, &m->High, 8);
        FPRINT(buf, "    st%u = 0x", i);
        for (int j = 9; j >= 0; j--) FPRINT(buf, "%02x", b[j]);
        FPRINT(buf, "\n");
    }

    FPRINT(buf, "\nSSE (XMM)\n");
    for (unsigned i = 0; i < 16; i++) {
        unsigned char b[16];
        psi_get_xmm_bytes(ctx, i, b);
        FPRINT(buf, "    xmm%-2u = ", i);
        psi_fprint_bytes_be(buf, b, 16);
        FPRINT(buf, "\n");
    }

    FPRINT(buf, "\nAVX (YMM)\n");
    if (!have_avx) {
        FPRINT(buf, "    <unavailable: host CPU does not report XSTATE_MASK_AVX%s>\n",
               self_no_xstate ? " for self-inspection on this Windows build" : "");
    } else {
        for (unsigned i = 0; i < 16; i++) {
            unsigned char b[32];
            psi_get_ymm_bytes(ctx, i, b);
            FPRINT(buf, "    ymm%-2u = ", i);
            psi_fprint_bytes_be(buf, b, 32);
            FPRINT(buf, "\n");
        }
    }

    FPRINT(buf, "\nAVX-512 (ZMM + k)\n");
    if (!have_avx512) {
        FPRINT(buf, "    <unavailable: host CPU does not report XSTATE_MASK_AVX512%s>\n",
               self_no_xstate ? " for self-inspection on this Windows build" : "");
    } else {
        for (unsigned i = 0; i < 32; i++) {
            unsigned char b[64];
            if (!psi_get_zmm_bytes(ctx, i, b)) {
                FPRINT(buf, "    zmm%-2u = <unavailable>\n", i);
                continue;
            }
            FPRINT(buf, "    zmm%-2u = ", i);
            psi_fprint_bytes_be(buf, b, 64);
            FPRINT(buf, "\n");
        }
        for (unsigned i = 0; i < 8; i++) {
            ULONG64 v = 0;
            if (!psi_get_kmask(ctx, i, &v)) {
                FPRINT(buf, "    k%u = <unavailable>\n", i);
                continue;
            }
            FPRINT(buf, "    k%u = 0x%016llx\n", i, (unsigned long long)v);
        }
    }
}

void handle_regdump(datap *parser) {
    // Probe for XSTATE availability. GetEnabledXStateFeatures is Win7 SP1+;
    // resolve dynamically so older hosts just fall back to plain CONTEXT.
    DWORD64 enabled = 0;
    HMODULE hkernel = GetModuleHandleA("kernel32.dll");
    if (hkernel) {
        typedef DWORD64 (WINAPI *GetEnabledXStateFeatures_t)(void);
        GetEnabledXStateFeatures_t fn = (GetEnabledXStateFeatures_t)
            GetProcAddress(hkernel, "GetEnabledXStateFeatures");
        if (fn) enabled = fn();
    }
    DWORD64 requested = enabled & (PSI_XSTATE_MASK_AVX | PSI_XSTATE_MASK_AVX512);

    PVOID    buf_raw = NULL;
    PCONTEXT ctx     = NULL;
    BOOL     have_xstate = FALSE;

    if (requested) {
        DWORD len = 0;
        InitializeContext(NULL, CONTEXT_ALL | CONTEXT_XSTATE, NULL, &len);
        if (len > 0) {
            buf_raw = LocalAlloc(0x0040 /* LPTR */, len);
            if (buf_raw) {
                PCONTEXT tmp = NULL;
                if (InitializeContext(buf_raw, CONTEXT_ALL | CONTEXT_XSTATE, &tmp, &len)
                    && tmp && SetXStateFeaturesMask(tmp, requested)) {
                    ctx = tmp;
                    have_xstate = TRUE;
                } else {
                    LocalFree(buf_raw);
                    buf_raw = NULL;
                }
            }
        }
    }

    if (!ctx) {
        buf_raw = LocalAlloc(0x0040 /* LPTR */, sizeof(CONTEXT));
        if (!buf_raw) {
            EPRINT("[-] psi regdump: LocalAlloc(CONTEXT) failed (GLE=0x%lx)\n",
                   GetLastError());
            return;
        }
        // LocalAlloc returns 16-byte-aligned heap memory on amd64, which
        // CONTEXT's M128A fields require.
        ctx = (PCONTEXT)buf_raw;
    }

    // Try RtlCaptureContext2 for XSTATE-aware self capture (Win10 20H1+).
    typedef VOID (NTAPI *RtlCaptureContext2_t)(PCONTEXT);
    RtlCaptureContext2_t capture2 = NULL;
    if (hkernel) {
        capture2 = (RtlCaptureContext2_t)GetProcAddress(hkernel, "RtlCaptureContext2");
    }

    // Set ContextFlags + xstate mask before capture so RtlCaptureContext2 knows
    // what to collect. Plain RtlCaptureContext ignores the mask but still
    // respects the CONTEXT_EX buffer layout.
    ctx->ContextFlags = have_xstate ? (CONTEXT_ALL | CONTEXT_XSTATE) : CONTEXT_ALL;
    if (have_xstate) SetXStateFeaturesMask(ctx, requested);

    if (capture2) capture2(ctx);
    else          RtlCaptureContext(ctx);

    BOOL self_xstate_ok = have_xstate && (capture2 != NULL);

    // Parse args after capture so rip/rsp reflect a point near entry.
    int   tid_i    = BeaconDataInt(parser);
    char *reg_name = BeaconDataExtract(parser, NULL);
    if (reg_name && *reg_name == 0) reg_name = NULL;

    DWORD self_tid      = GetCurrentThreadId();
    DWORD effective_tid = (tid_i < 0) ? self_tid : (DWORD)tid_i;
    BOOL  is_self       = (effective_tid == self_tid);

    BOOL avx_available    = FALSE;
    BOOL avx512_available = FALSE;

    if (!is_self) {
        HANDLE h = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME |
                              THREAD_QUERY_INFORMATION, FALSE, effective_tid);
        if (!h) {
            EPRINT("[-] psi regdump: OpenThread(%lu) failed (GLE=0x%lx)\n",
                   (unsigned long)effective_tid, GetLastError());
            LocalFree(buf_raw);
            return;
        }

        if (SuspendThread(h) == (DWORD)-1) {
            DWORD gle = GetLastError();
            CloseHandle(h);
            LocalFree(buf_raw);
            EPRINT("[-] psi regdump: SuspendThread(%lu) failed (GLE=0x%lx)\n",
                   (unsigned long)effective_tid, gle);
            return;
        }

        ctx->ContextFlags = have_xstate ? (CONTEXT_ALL | CONTEXT_XSTATE) : CONTEXT_ALL;
        if (have_xstate) SetXStateFeaturesMask(ctx, requested);

        BOOL  ok  = GetThreadContext(h, ctx);
        DWORD gle = ok ? 0 : GetLastError();

        // Cleanup invariant: resume + close before any formatting or early-exit.
        ResumeThread(h);
        CloseHandle(h);

        if (!ok) {
            LocalFree(buf_raw);
            EPRINT("[-] psi regdump: GetThreadContext(%lu) failed (GLE=0x%lx)\n",
                   (unsigned long)effective_tid, gle);
            return;
        }

        avx_available    = have_xstate && (requested & PSI_XSTATE_MASK_AVX)    != 0;
        avx512_available = have_xstate && (requested & PSI_XSTATE_MASK_AVX512) == PSI_XSTATE_MASK_AVX512;
    } else {
        avx_available    = self_xstate_ok && (requested & PSI_XSTATE_MASK_AVX)    != 0;
        avx512_available = self_xstate_ok && (requested & PSI_XSTATE_MASK_AVX512) == PSI_XSTATE_MASK_AVX512;
    }

    // Filter mode: match the alias then print just that register.
    if (reg_name) {
        const psi_reg_alias_t *a = psi_find_alias(reg_name);
        if (!a) {
            EPRINT("[-] psi regdump: unknown register '%s' (try rax..r15, xmm0..xmm15, "
                   "ymm0..ymm15, zmm0..zmm31, k0..k7, cs/ds/ss/fs/gs/es, dr0..dr7, "
                   "rflags, mxcsr, fctrl/fstat/ftag/fop, st0..st7)\n", reg_name);
            LocalFree(buf_raw);
            return;
        }

        formatp fb;
        BeaconFormatAlloc(&fb, 1024);
        FPRINT(&fb, "[+] psi regdump tid=%lu %s\n",
               (unsigned long)effective_tid, a->name);
        psi_print_single(&fb, ctx, a, avx_available, avx512_available);
        FLUSHFREE(&fb);
        LocalFree(buf_raw);
        return;
    }

    // Full dump.
    formatp fb;
    BeaconFormatAlloc(&fb, 16384);
    FPRINT(&fb, "[+] psi regdump tid=%lu (%s)\n\n",
           (unsigned long)effective_tid,
           is_self ? "self capture" : "suspended snapshot");
    psi_print_full(&fb, ctx, avx_available, avx512_available,
                   is_self && have_xstate && !self_xstate_ok);
    FLUSHFREE(&fb);
    LocalFree(buf_raw);
}
