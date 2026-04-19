#pragma once
#include "common.h"
#include "kernel32.imp.h"

#ifndef __KERNEL32_IMPORTS__
#define __KERNEL32_IMPORTS__

DFR(Kernel32, GetLastError)
#define GetLastError Kernel32$GetLastError

DFR(Kernel32, GetCurrentThreadId)
#define GetCurrentThreadId Kernel32$GetCurrentThreadId

DFR(Kernel32, GetCurrentProcess)
#define GetCurrentProcess Kernel32$GetCurrentProcess

DFR(Kernel32, ReadProcessMemory)
#define ReadProcessMemory Kernel32$ReadProcessMemory

DFR(Kernel32, LocalAlloc)
#define LocalAlloc Kernel32$LocalAlloc

DFR(Kernel32, LocalFree)
#define LocalFree Kernel32$LocalFree

DFR(Kernel32, VirtualQuery)
#define VirtualQuery Kernel32$VirtualQuery

#endif