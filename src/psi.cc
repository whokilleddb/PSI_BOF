#include "psi.h"

// Unity build: Beacon loads a single COFF .obj, and the MSVC/clang-cl toolchain
// has no partial-link step. Pulling the sibling .cc files in here compiles them
// all as one translation unit, so the Makefile only has to drive psi.cc.
#include "crt_stubs.cc"
#include "psi_util.cc"
#include "psi_ba.cc"
#include "psi_addr.cc"

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
