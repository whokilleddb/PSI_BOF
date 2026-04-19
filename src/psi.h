#pragma once
#include "common.h"

#ifndef __PSI_H__
#define __PSI_H__

// winternl.h ships a stripped LDR_DATA_TABLE_ENTRY that hides BaseDllName and
// SizeOfImage. Redeclare the real layout under a PSI_ prefix so we can pull the
// fields we need directly from the loader list.
// ref: https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_data_table_entry.htm
typedef struct _PSI_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY     InLoadOrderLinks;
    LIST_ENTRY     InMemoryOrderLinks;
    LIST_ENTRY     InInitializationOrderLinks;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} PSI_LDR_DATA_TABLE_ENTRY, *PPSI_LDR_DATA_TABLE_ENTRY;

// psi_util.cc
int    psi_streq(const char *a, const char *b);
int    psi_basename_matches(const UNICODE_STRING *u, const char *name);
void   psi_us_to_ansi(const UNICODE_STRING *u, char *out, size_t cap);
int    psi_parse_hex_u64(const char *s, unsigned long long *out);
size_t psi_type_size(const char *type);
const char *psi_mem_state_str(DWORD state);
const char *psi_mem_type_str(DWORD type);
void   psi_print_protect(formatp *fb, DWORD prot);

// subcommand handlers
void handle_addr(datap *parser);
void handle_lm(datap *parser);
void handle_lt(datap *parser);
void handle_meminfo(datap *parser);
void handle_regdump(datap *parser);

#endif