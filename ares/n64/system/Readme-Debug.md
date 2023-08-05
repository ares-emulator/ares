# Debug Server

The file `debug-server.cpp` provides a simple HTTP server to expose various debugging commands.

Commands are loosely based on gdb, although modified for easier use.

<br>

## Sending Commands

Anything that can send HTTP requests can be used to execute commands, in this doc, curl is used.<br>
The payload is expected to be the body of the request, while the method is always `POST`.

### Arguments

Many commands have numeric arguments, all of them are expected to be hexadecimal.<br>
A "`0x`" prefix can be used, but is optional.

### Multiplexing

To avoid overhead, multiple commands can be send at the same time, separated by a newline (`\n`).<br>
Results will be returned in multiple lines too, keeping the same order as in the request.

### Error handling

If a command fails, `ERR` will be returned.<br>
Note that there are no "transactions", if one of multiple commands failed, the rest will still execute.

<br>

## Commands
<br>

### Memory-Bus Read

Command:
`read [count] [typeSize] [address]`

Example: (reads 8 u16-values starting from address 0x801EF90A):
```sh
curl -X POST 127.0.0.1:9123 -d 'read 0x8 2 0x801EF90A'
```
Response:
```sh
12 64 1fde 100 0 ff00 0 ff00
```

Example: (read various types from multiple addresses):
```sh
curl -X POST 127.0.0.1:9123 -d 'read 4 4 0x802B0000
read 4 2 0x800AFF00
read 4 1 0x800A4000'
```
Response:
```sh
409113 4421ece1 21001f81 babcff 
305 c023 18 c080 
1 ed 50 0 
```

<br>

### Memory-Bus Write

Command:
`write [count] [typeSize] [address] [value, ...]`

Example: (writes the u32 value 0x11223344 to address 0x801EF90A):
```sh
curl -X POST 127.0.0.1:9123 -d 'write 1 4 0x801EF90A 0x11223344'
```
Response:
```sh
OK
```

Example: (writes 4 u8 values to address 0x801EF90A):
```sh
curl -X POST 127.0.0.1:9123 -d 'write 4 1 0x801EF90A 0x11 0x22 0x33 0x44'
```
Response:
```sh
OK
```