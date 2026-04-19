# ProcessInspect BOF

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/whokilleddb)

A CS BOF which can be used to inspect process memory, addresses and symbols! 

## BOF commands

## lm 

The `lm` command lists loaded modules in the current process. When invoked without arguments it returns every loaded DLL; when given a DLL name it returns details for just that module (base address, entry point, size, full path).

**Syntax**

```
psi lm [dll]
```

Parameters:

- `dll` (optional): The name of a loaded DLL to look up. If omitted, every loaded module is listed.

Example:

```
psi lm
psi lm ntdll.dll
psi lm kernel32.dll
```

## addr 

The `addr` command returns the value at a given address

**Syntax**:

```
psi addr <address> <type> [count]
```

- `address`: The address to inspect in hex
- `type`: This controls how to treat the type of data at the given address. Valid values are: BYTE, WORD, DWORD, LPVOID. Therefore, if a user provides a value `DWORD`, then a chunck of `size(DWORD)` is read at that address. Each chunk is printed as a `0x`-prefixed little-endian value whose width equals the type size (e.g. a `WORD` row groups as `0xaabb 0xccdd ...`). Type names are matched case-insensitively. Same-sized Win32 aliases (`int`, `ULONG`, `ULONG64`, `HANDLE`, etc.) are intentionally omitted — use `BYTE`/`WORD`/`DWORD`/`LPVOID` for 1/2/4/8-byte reads.
- `count`: How many chuncks to read. This argument is optional and by default set to one 

The output should be a hexdump starting at the given address and chuncks grouped together

Example:

```
psi addr 0xdeadbeef DWORD 2 
psi addr 0xdeadbeef LPVOID
psi addr 0xdeadbeef WORD 8
```

## lt

The `lt` command prints every thread in the current process along with its TID, base/delta priority, TEB base address, and exit status. The Win32 start address (the value passed to `CreateThread`) is intentionally omitted: `NtQueryInformationThread(ThreadQuerySetWin32StartAddress)` returns `STATUS_ACCESS_DENIED` (`0xC0000022`) on modern Windows even for threads in the calling process, so the command reports what can be read reliably. `ExitStatus=0x103` (`STATUS_PENDING`) marks a live thread; any other value is an exited thread that hasn't been reaped. The TEB address pairs with `psi addr <teb> LPVOID` for quick triage.

**Syntax**

```
psi lt
```

Example:

```
psi lt
```

## regdump

The `regdump` command dumps the CPU register state of a thread in the current process. A thread id (`tid`) is required: `0` selects the calling Beacon thread (captured via `RtlCaptureContext` / `RtlCaptureContext2`); any non-zero value selects another thread, which is suspended only for the duration of the snapshot and resumed before anything is printed. If an optional register name is supplied, only that register is emitted; otherwise the full state is dumped, including the x64 GPR set plus `rflags` (with a flag-letter breakdown), segment registers, debug registers, `mxcsr`, x87 `fctrl`/`fstat`/`ftag`/`fop`/`st0..st7`, SSE (`xmm0..xmm15`), AVX/AVX2 (`ymm0..ymm15`), and AVX-512 (`zmm0..zmm31` + `k0..k7`) — the last three gated on the host's `GetEnabledXStateFeatures` report. Register names are matched case-insensitively and accept sub-width aliases (`rax`/`eax`/`ax`/`ah`/`al` all resolve against the same underlying GPR; likewise `r8`/`r8d`/`r8w`/`r8b`, etc.).

**Syntax**:

```
psi regdump <tid> [register]
```

- `tid`: Thread id whose context to dump. `0` means the current (calling) thread; on older Windows builds without `RtlCaptureContext2`, the self-inspection YMM/ZMM sections are reported as unavailable rather than fabricated.
- `register`: optional register name to filter the output to a single register.

Example:

```
psi regdump 0
psi regdump 0 rax
psi regdump 12345 rip
psi regdump 12345 ymm0
```

## Building BOFs

To compile a BOF, update the `src/hello.cc` file or the `Makefile` to reflect the necessary changes and then type:

```
$ docker-compose up --build
```

_Thanks to [Steve](https://x.com/0xTriboulet) on Twitter for the inspiration!_