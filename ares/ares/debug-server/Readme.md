# Debug Server

The file `server.cpp` adds a gdb-server compatible with several GDB versions and IDEs like VScode and CLion.<br>
It is implemented as a standalone server independent of any specific system, and even ares itself.<br>
This allows for easy integration with systems without having to worry about the details of GDB itself.<br>
 
Managing the server itself, including the underlying TCP connection, is done by ares.<br>
System specific logic is handled via (optional) call-backs that a can be registered,<br>
as well as methods to report events to GDB.

The overall design of this server is to be as neutral as possible.<br>
Meaning that things like stopping, stepping and reading memory should not affect the game.<br>
This is done to make sure that games behave the same as if they were running without a debugger, down to the cycle.<br> 

## Integration Guide
This section describes how to implement the debugger for a system in ares.<br>
It should not be necessary to modify the server itself, or to know much about the GDB protocol.<br>
Simply registering callbacks and reporting events are enough to get the full set of features working.<br>

For a minimal working debugging session, register/memory reads and a way to report the PC are required.<br>
Although implementing as much as possible is recommended to make GDB more stable.

Interactions with the server can be split in three categories:
- **Hooks:** lets GDB call functions in your ares system (e.g.: memory read)
- **Report-functions:** notify GDB about events (e.g.: exceptions)
- **Status functions:** helper to check the GDB status (e.g.: are breakpoints set or not)

Hooks can be set via setting the callbacks in `GDB::server.hooks.XXX`.<br>
Report functions are prefixed `GDB::server.reportXXX()`, and status functions a documented here separately.<br>
All hooks/report/status functions can be safely set or called even if the server is not running.<br>

As an example of a fictional system, this is what a memory read could look like:
```cpp
GDB::server.hooks.regRead = [](u32 regIdx) {
  return hex(cpu.readRegister(regIdx), 16, '0');
};
``` 
Or the main execution loop:
```cpp
while(!endOfFrame && GDB::server.reportPC(cpu.getPC())) {
  cpu.step();
}
```

For a real reference implementation, you can take a look at the N64 system.<br>

## Hooks

### Memory Read - `hooks.read = (u64 address, u32 byteCount) -> string`
Reads `byteCount` bytes from `address` and returns them as a hex-string.<br>
Both the hex-encoding / single-byte reads are dictated by the GDB protocol.<br>

It is important to implement this in a neutral way: no exceptions and status changes.<br>
The GDB-client may issue reads from any address at any point while halted.<br>
If not handled properly, this can cause game crashes or different emulation behavior.<br>

If your system emulates cache, make sure to also handle this here.<br>
A read must be able to see the cache, but never cause flush.<br>

Example response (reading 3 bytes): `A1B200`

### Memory Write - `hooks.write = (u64 address, u32 unitSize, u64 value) -> void`
Writes `value` of byte-size `unitSize` to `address`.<br>
For example, writing a 32-bit value would issue a call like this: `write(0x80001230, 4, 0x0000000012345678)`.<br>
Contrary to read, this is not required to be neutral, and is allowed to cause exceptions.<br>

If your system emulates cache, make sure to also handle this here.<br>
The write should behave the same as if it was done via a CPU instruction, incl. flushing the cache if needed.<br>

### Normalize Address - `hooks.normalizeAddress = (u64 address) -> u64`
Normalizes an address into something that makes it comparable.<br>
This is only used for memory-watchpoints, which needs to compare what GDB send to what ares has internally.<br>
If your system has virtual addresses or masks, this should de-virtualize it.<br>

It's OK to not set this function, or to simply return the input untouched.<br>
In case that memory-watchpoint are not working, this is probably the place to fix it.<br>

Example implementation:
```cpp
GDB::server.hooks.normalizeAddress = [](u64 address) {
  return address & 0x0FFF'FFFF;
};
```

### Register Read - `hooks.regRead = (u32 regIdx) -> string`
Reads a single register at `regIdx` and returns it as a hex-string.<br>
The size of the hex-string is dictated by the specific architecture.<br>

Same as for memory-read, this must be implemented in a neutral way.<br>
Any invalid register can be returned as zero.<br>

Example reponse: `00000000000123AB`

### Register Write - `hooks.regWrite = (u32 regIdx, u64 regValue) -> bool`

Writes the value `regValue` to the register at `regIdx`.<br>
This write is allowed to have side effects.<br>

If the specific register is not writable or doesn't exist, `false` must be returned.<br>
On success, `true` must be returned.<br>

### Register Read (General) - `hooks.regReadGeneral = () -> string`
Most common way for GDB to read registers, this fetches all registers at once.<br>
The amount and order of registers is dictated by the specific architecture and GDB.<br>
When implementing this, GDB will usually complain if the order/size is incorrect.<br>

Same as for single reads, this must be implemented in a neutral way.<br>

Other than that, this can be implemented by looping over `hooks.regRead` and returning a concatenated string.<br>
Example response: `0000000000000000ffffffff8001000000000000000000420000000000000000000000000000000100000`...

### Register Write (General) - `hooks.regWriteGeneral = (const string &regData) -> void`
Writes all registers at once, this happens very rarely.<br>
The format of `regData` is the same as the response of `hooks.regReadGeneral`.<br>
Any register that is not writable or doesn't exist can be ignored.<br>

### Emulator Cache - `hooks.emuCacheInvalidate = (u64 address) -> void`
Should invalidate the emulator's cache at `address`.<br>
This is only necessary if you have a re-compiler or some form of instruction cache.<br>

### Target XML - `hooks.targetXML = () -> string`
Provides an XML description of the target system.<br>
The XML must not contain any newlines, and should be as short as possible.<br>
If the client has access to an `.elf` file, this will be mostly ignored.

Example implementation:
```cpp
GDB::server.hooks.targetXML = []() -> string {
  return "<target version=\"1.0\">"
    "<architecture>mips:4000</architecture>"
  "</target>";
};
```

Documentation: https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
<hr>

## API Usage

This API can also be used without GDB, which allows for more use cases.<br>
For example, you can write automated tooling or custom debugging UIs.<br>
To make access easier, no strict checks are performed.<br>
This means that the handshake protocol is optional, and checksums are not verified.

### TCP
TCP connections behave the same way as a GDB session.<br>
The connection is kept open the entire time, and commands are sent sequentially, each waiting for an response before sending the next command.

However, it is possible to send commands even if the game is still running,
this allows for real-time data access.

Keep in minds that the server uses the RDP-commands, which are different from what you would type into a GDB client.<br>
For a list of all commands, see: https://sourceware.org/gdb/onlinedocs/gdb/Packets.html#Packets

As an example, reading from memory would look like this:
```
$m8020a504,100#00
```
This reads 100 bytes from address `0x8020a504`, the `$` and `#` define the message start/end, and the `00` is the checksum (which is not checked).

One detail, and security check, is that new connections must send `+` as the first byte in the first payload.<br>
It's also a good idea to send a proper disconnect-command before closing the socket.<br>
Otherwise, the debugger will not accept new connections until a reset or restart occurs.