# ProcessInspect BOF

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/whokilleddb)

A CS BOF which can be used to inspect process memory, addresses and symbols! 

## Overview of commands 

| Command | Description |
|-------|----------|
| lm | list all loaded modules along with base address, entry point, and more|
| addr | Print the bytes at a given memory address |
| meminfo | Print information for a given memory address |
| lt | Print a list of all active threads in the current process along with some basic information |
| regdump | Print the contents of a thread's registers | 

-----

_Send patches! Send recommendations for more commands!_

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

## meminfo

The `meminfo` command returns various information about a given memory address including:
- Name of the module backing the address if any
- Memory permissions of the memory region R/W/X
- Any associated symbols at that address fetched from microsoft symbol server
- Other information similar to windbg !address command 

**Syntax**:

```
psi meminfo <address> 
```

- `address`: The address to inspect in hex

Example:

```
psi addr 0xdeadbeef
```

## lt 

The `lt` command prints all the threads of the current process along with their TID and entrypoint

**Syntax**

```
psi lt
```

## regdump

The `regdump` command dumps CPU registers for a thread. A thread id (`tid`) is required: `0` selects the calling Beacon thread; any non-zero value selects another thread in the current process (which is suspended only for the duration of the snapshot). If a register name is supplied, only that register is printed; otherwise every supported register is dumped, including the full x64 GPR set plus RFLAGS, segment registers, debug registers, SSE (XMM), AVX/AVX2 (YMM), and AVX-512 (ZMM / kmask) — the last three gated on the host's CPU feature support. Register names are matched case-insensitively and accept sub-width aliases (e.g. `eax`, `ax`, `ah`, `al` all resolve against the same underlying GPR).

**Syntax**:

```
psi regdump <tid> [register]
```

- `tid`: Thread id whose context to dump. `0` means the current (calling) thread.
- `register`: optional register name to filter the output to a single register.

Example:

```
psi regdump 0
psi regdump 0 rax
psi regdump 12345 rip
psi regdump 12345 ymm0
```


# Building BOFs

To compile a BOF, update the `src/hello.cc` file or the `Makefile` to reflect the necessary changes and then type:

```
$ docker-compose up --build
```

Built using the template from: https://github.com/whokilleddb/cs-bof-template