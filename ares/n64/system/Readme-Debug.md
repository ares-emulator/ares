# Debug Server

The file `debug-server.cpp` provides a simple HTTP server to expose various debugging commands.

Commands are similar to gdb, although modified for easier use.

<br>

## Sending Commands

Anything that can send HTTP requests can be used to execute commands, in this doc, curl is used.

The payload is expected to be the body of the request, while the method is always `POST`.

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
<br>

### Memory-Bus Write

Command:
`write [count] [typeSize] [address] [value, ...]`

@TODO: implement multi-value write

Example: (writes the u32 value 0x11223344 to address 0x801EF90A):
```sh
curl -X POST 127.0.0.1:9123 -d 'write 1 4 0x801EF90A 0x11223344'
```
Response:
```sh
OK
```