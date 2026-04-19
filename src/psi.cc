#include "psi.h"

// Unity build: Beacon loads a single COFF .obj, and the MSVC/clang-cl toolchain
// has no partial-link step. Pulling the sibling .cc files in here compiles them
// all as one translation unit, so the Makefile only has to drive psi.cc.
#include "crt_stubs.cc"
#include "psi_util.cc"
#include "psi_addr.cc"
#include "psi_lm.cc"
#include "psi_lt.cc"
#include "psi_regdump.cc"

static void handle_cmdline(const char *subcmd, datap *parser) {
    if (!subcmd) {
        EPRINT("[-] psi: missing subcommand\n");
        return;
    }

    if      (psi_streq(subcmd, "addr"))    handle_addr(parser);
    else if (psi_streq(subcmd, "lm"))      handle_lm(parser);
    else if (psi_streq(subcmd, "lt"))      handle_lt(parser);
    else if (psi_streq(subcmd, "regdump")) handle_regdump(parser);
    else    EPRINT("[-] psi: unknown subcommand '%s'\n", subcmd);
}

extern "C" void go(char *args, int length) {
    datap parser;
    BeaconDataParse(&parser, args, length);

    char *subcmd = BeaconDataExtract(&parser, NULL);
    handle_cmdline(subcmd, &parser);
}
