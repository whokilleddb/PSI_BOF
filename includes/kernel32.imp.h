#pragma once
#include "common.h"
#include "kernel32.imp.h"

#ifndef __KERNEL32_IMPORTS__
#define __KERNEL32_IMPORTS__

DFR(Kernel32, GetLastError)
#define GetLastError Kernel32$GetLastError

DFR(Kernel32, GetCurrentThreadId)
#define GetCurrentThreadId Kernel32$GetCurrentThreadId

DFR(Kernel32, GetCurrentProcessId)
#define GetCurrentProcessId Kernel32$GetCurrentProcessId

DFR(Kernel32, GetCurrentProcess)
#define GetCurrentProcess Kernel32$GetCurrentProcess

DFR(Kernel32, ReadProcessMemory)
#define ReadProcessMemory Kernel32$ReadProcessMemory

DFR(Kernel32, LocalAlloc)
#define LocalAlloc Kernel32$LocalAlloc

DFR(Kernel32, LocalFree)
#define LocalFree Kernel32$LocalFree

DFR(Kernel32, CreateToolhelp32Snapshot)
#define CreateToolhelp32Snapshot Kernel32$CreateToolhelp32Snapshot

DFR(Kernel32, Thread32First)
#define Thread32First Kernel32$Thread32First

DFR(Kernel32, Thread32Next)
#define Thread32Next Kernel32$Thread32Next

DFR(Kernel32, OpenThread)
#define OpenThread Kernel32$OpenThread

DFR(Kernel32, CloseHandle)
#define CloseHandle Kernel32$CloseHandle

DFR(Kernel32, SuspendThread)
#define SuspendThread Kernel32$SuspendThread

DFR(Kernel32, ResumeThread)
#define ResumeThread Kernel32$ResumeThread

DFR(Kernel32, GetThreadContext)
#define GetThreadContext Kernel32$GetThreadContext

DFR(Kernel32, RtlCaptureContext)
#define RtlCaptureContext Kernel32$RtlCaptureContext

DFR(Kernel32, InitializeContext)
#define InitializeContext Kernel32$InitializeContext

DFR(Kernel32, SetXStateFeaturesMask)
#define SetXStateFeaturesMask Kernel32$SetXStateFeaturesMask

DFR(Kernel32, LocateXStateFeature)
#define LocateXStateFeature Kernel32$LocateXStateFeature

DFR(Kernel32, GetEnabledXStateFeatures)
#define GetEnabledXStateFeatures Kernel32$GetEnabledXStateFeatures

DFR(Kernel32, GetModuleHandleA)
#define GetModuleHandleA Kernel32$GetModuleHandleA

DFR(Kernel32, GetProcAddress)
#define GetProcAddress Kernel32$GetProcAddress

#endif