# Debug Server

The file `debug-server.cpp` provides a simple gdbserver compatible with GDB and IDEs.<br>
In addition to TCP, requests can also be made via HTTP if a token is provided.

@TODO: port and token settings

<br>

## Integration Guide
This section describes how to implement the debugger for a system in ares.

## Custom Commands

@TODO: see above

## API Usage

This API can also be used without GDB, which allows for more use cases.<br>
For example, you can write automated tooling or custom debugging UIs.<br>
To make access easier, no strict checks are performed.<br>
This means that the handshake protocol is optional, and checksums are not verified.

### TCP
TCP connections behave the same way as a GDB session.<br>
The connection is kept open the entire time, and commands are sent sequentially, each waiting for an response before sending the next command.

Keep in minds that this uses the server-commands, which are different from what you would type into a GDB client.<br>
For a list of all commands, see: https://sourceware.org/gdb/onlinedocs/gdb/Packets.html#Packets

As an example, reading from memory would look like this:
```
$m8020a504,100#00
```
This this reads 100 bytes from address `0x8020a504`, the `$` and `#` define the message start/end, and the `00` is the checksum (which is not checked).

### HTTP

TODO