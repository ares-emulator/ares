"""Export real FastMCP schemas without connecting to or executing an emulator."""
import asyncio
import hashlib
import importlib.metadata
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
source = Path(sys.argv[1]).resolve()
spec = importlib.util.spec_from_file_location('ares_reference_schema', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

async def main():
    tools = await module.mcp.list_tools()
    print(json.dumps({
        'source': 'tools/mcp/ares_mcp.py',
        'commit': subprocess.check_output(['git', '-C', str(source.parent), 'rev-parse', 'HEAD'], text=True).strip(),
        'sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'mcp_version': importlib.metadata.version('mcp'),
        'tools': sorted([{'name': tool.name, 'inputSchema': tool.inputSchema} for tool in tools], key=lambda tool: tool['name']),
    }, indent=2))

asyncio.run(main())
