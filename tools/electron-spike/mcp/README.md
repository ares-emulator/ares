# MCP adapter boundary

Task 3 will add the Node stdio entry point here. It will consume the shared
`services/` contracts/client and connect to Electron main's explicitly selected
local instance. It must not spawn an emulator or put diagnostics on stdout.

`parity-ledger.json` is the frozen 68-tool source inventory. Implementation and
schema parity remain separate gates; task 1 does not expose MCP tools.
