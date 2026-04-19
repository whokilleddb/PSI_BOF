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
- `type`: This controls how to treat the type of data at the given address. Valid values are: WORD, DWORD, int, LPVOID, ULONG, ULONG64, HANDLE. Therefore, if a user provides a value `DWORD`, then a chunck of `size(DWORD)` is read at that address
- `count`: How many chuncks to read. This argument is optional and by default set to one 

The output should be a hexdump starting at the given address and chuncks grouped together

Example:

```
psi addr 0xdeadbeef DWORD 2 
psi addr 0xdeadbeef LPVOID
psi addr 0xdeadbeef HANDLE 5
```

## Building BOFs

To compile a BOF, update the `src/hello.cc` file or the `Makefile` to reflect the necessary changes and then type:

```
$ docker-compose up --build
```

_Thanks to [Steve](https://x.com/0xTriboulet) on Twitter for the inspiration!_