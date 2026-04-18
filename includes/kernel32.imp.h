#pragma once
#include "common.h"
#include "kernel32.imp.h"

#ifndef __KERNEL32_IMPORTS__
#define __KERNEL32_IMPORTS__

DFR(Kernel32, GetLastError)
#define GetLastError Kernel32$GetLastError

DFR(Kernel32, GetCurrentThreadId)
#define GetCurrentThreadId Kernel32$GetCurrentThreadId

#endif