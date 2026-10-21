`rspq-libdragon.inc` embeds the JSON descriptor from `ares-64` commit
`1c9b9637476a83f7b80d3db1115a0f71529820d9`,
`desktop-ui/ui/rsp-overlays/rspq-libdragon.json`, inside a C++ raw string.
It is compiled into the host so copied packages do not need checkout files.
A bounded JSON descriptor beside a companion ELF overrides it at ROM load.
