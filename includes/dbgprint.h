/*
    More macros to help with printing

*/

#pragma once
#include "common.h"

#ifndef __DBGPRINT__
#define __DBGPRINT__

#define PRINT(format, ...) BeaconPrintf(CALLBACK_OUTPUT, OBFUSCATE_STRING(format), ##__VA_ARGS__)
#define EPRINT(format, ...) BeaconPrintf(CALLBACK_ERROR, OBFUSCATE_STRING(format), ##__VA_ARGS__)

#define ERR_PRINT(func)      EPRINT("%s() failed at %s:%d with error: 0x%lx\n", func, OBFUSCATE_STRING(__FILE__), __LINE__, GetLastError())
#define NTEPRINT(x, status)  EPRINT("%s() failed at %s:%d with error: 0x%lx\n", x, OBFUSCATE_STRING(__FILE__), __LINE__, status)
#define MALLOC_E             EPRINT("malloc() failed at %s:%d\n", OBFUSCATE_STRING(__FILE__), __LINE__)
#define DBG                  PRINT("[+] WORKS AT %s:%d\n", OBFUSCATE_STRING(__FILE__), __LINE__)
#define FPRINT(pbuffer, format, ...)  BeaconFormatPrintf(pbuffer, OBFUSCATE_STRING(format), ##__VA_ARGS__)
#define FLUSH(p_buffer)         PRINT("%s\n", BeaconFormatToString(p_buffer, NULL))
#define FLUSHFREE(p_buffer)     FLUSH(p_buffer); BeaconFormatFree(p_buffer);
#define PCURRTID                PRINT("[+] Current thread: %ld", GetCurrentThreadId())

#endif
