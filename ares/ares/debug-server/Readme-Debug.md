# Debug Server

The file `debug-server.cpp` provides a simple TCP/HTTP gdbserver to expose various debugging commands.<br/>
In addition to TCP, requests can also be made via HTTP if a token is provided.

@TODO: port and token settings

<br>

## Supported Commands

@TODO: client side commands

## Core integration-guide

@TODO: how to add debugger to a core

## Custom Commands

@TODO: see above

## API Usage

This API can also be used without GDB, which allows for more use cases.<br>
For example, you can write automated tooling or custom debugging UIs.<br>
To make access easier, no strict checks are performed.<br>
This means that the handshake protocol is optional, and checksums are not verified.

@TODO: examples