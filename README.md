# ProcessInspect BOF

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/whokilleddb)

A CS BOF which can be used to inspect process memory, addresses and symbols! 

## BOF commands

## ba 

The `ba` command returns the base address of a DLL in the current process. 

**Syntax**:

```
psi ba dllname
```

Parameters:

- `dll_name`: The name of the DLL to find the base address of

Example:

```
psi ba ntdll.dll
psi ba kernel32.dll
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