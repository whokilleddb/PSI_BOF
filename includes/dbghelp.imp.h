#pragma once
#include <dbghelp.h>
#include "common.h"

#ifndef __DBGHELP_IMPORTS__
#define __DBGHELP_IMPORTS__

DFR(DbgHelp, SymInitialize)
#define SymInitialize DbgHelp$SymInitialize

DFR(DbgHelp, SymCleanup)
#define SymCleanup DbgHelp$SymCleanup

DFR(DbgHelp, SymSetOptions)
#define SymSetOptions DbgHelp$SymSetOptions

DFR(DbgHelp, SymLoadModuleEx)
#define SymLoadModuleEx DbgHelp$SymLoadModuleEx

DFR(DbgHelp, SymUnloadModule64)
#define SymUnloadModule64 DbgHelp$SymUnloadModule64

DFR(DbgHelp, SymFromAddr)
#define SymFromAddr DbgHelp$SymFromAddr

#endif
