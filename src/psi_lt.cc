#include "psi.h"

// ThreadQuerySetWin32StartAddress (class 9) is the only documented route to a
// thread's Win32 entrypoint, and it returns STATUS_ACCESS_DENIED (0xC0000022)
// on many modern Windows builds even from inside the owning process. Fall back
// to ThreadBasicInformation (class 0), which is reliably queryable with
// THREAD_QUERY_LIMITED_INFORMATION and yields TEB + ExitStatus.

// winternl.h in recent SDKs no longer exposes THREAD_BASIC_INFORMATION —
// redeclare it locally under a PSI_ prefix, same pattern as PSI_LDR_DATA_TABLE_ENTRY.
#define PSI_ThreadBasicInformation 0

typedef struct _PSI_THREAD_BASIC_INFORMATION {
    LONG      ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    LONG      Priority;
    LONG      BasePriority;
} PSI_THREAD_BASIC_INFORMATION;

#define PSI_NT_SUCCESS(s) ((LONG)(s) >= 0)

void handle_lt(datap *parser) {
    (void)parser;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        EPRINT("[-] psi lt: CreateToolhelp32Snapshot failed (GLE=0x%lx)\n",
               GetLastError());
        return;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    if (!Thread32First(snap, &te)) {
        EPRINT("[-] psi lt: Thread32First failed (GLE=0x%lx)\n", GetLastError());
        CloseHandle(snap);
        return;
    }

    DWORD self_pid = GetCurrentProcessId();

    formatp buf;
    BeaconFormatAlloc(&buf, 32768);

    FPRINT(&buf, "[+] Threads of PID %lu\n", (unsigned long)self_pid);

    unsigned long count       = 0;
    unsigned long unavailable = 0;

    do {
        if (te.th32OwnerProcessID != self_pid) continue;
        count++;

        DWORD tid      = te.th32ThreadID;
        LONG  base_pri = te.tpBasePri;
        LONG  delta    = te.tpDeltaPri;

        HANDLE h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
        if (!h) {
            unavailable++;
            FPRINT(&buf,
                   "    TID=%-8lu BasePri=%-2ld DeltaPri=%-3ld "
                   "<OpenThread failed, GLE=0x%lx>\n",
                   (unsigned long)tid, (long)base_pri, (long)delta,
                   GetLastError());
            continue;
        }

        PSI_THREAD_BASIC_INFORMATION tbi;
        LONG status = NtQueryInformationThread(
            h, (THREADINFOCLASS)PSI_ThreadBasicInformation,
            &tbi, sizeof(tbi), NULL);
        CloseHandle(h);

        if (!PSI_NT_SUCCESS(status)) {
            unavailable++;
            FPRINT(&buf,
                   "    TID=%-8lu BasePri=%-2ld DeltaPri=%-3ld "
                   "<NtQueryInformationThread NTSTATUS=0x%lx>\n",
                   (unsigned long)tid, (long)base_pri, (long)delta,
                   (unsigned long)status);
            continue;
        }

        FPRINT(&buf,
               "    TID=%-8lu BasePri=%-2ld DeltaPri=%-3ld "
               "TEB=0x%016llx ExitStatus=0x%lx\n",
               (unsigned long)tid, (long)base_pri, (long)delta,
               (unsigned long long)(uintptr_t)tbi.TebBaseAddress,
               (unsigned long)tbi.ExitStatus);
    } while (Thread32Next(snap, &te));

    CloseHandle(snap);

    if (unavailable > 0) {
        FPRINT(&buf,
               "[+] Total %lu threads in PID %lu (%lu enriched fields "
               "unavailable — possible EDR hook on NtQueryInformationThread)",
               count, (unsigned long)self_pid, unavailable);
    } else {
        FPRINT(&buf, "[+] Total %lu threads in PID %lu",
               count, (unsigned long)self_pid);
    }

    FLUSHFREE(&buf);
}
