#pragma once
#include "common.h"

#ifndef __PSI_H__
#define __PSI_H__

// winternl.h ships a stripped LDR_DATA_TABLE_ENTRY that hides BaseDllName and
// SizeOfImage. Redeclare the real layout under a PSI_ prefix so we can pull the
// fields we need directly from the loader list.
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

#endif