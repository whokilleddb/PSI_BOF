#pragma once
#include "common.h"

#ifndef __NTDLL_IMPORTS__
#define __NTDLL_IMPORTS__

DFR(Ntdll, NtQueryInformationThread)
#define NtQueryInformationThread Ntdll$NtQueryInformationThread

#endif
