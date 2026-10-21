R"ARES_RSPQ({
  "name": "libdragon RSPQ",
  "banner": "Dragon RSP Queue",
  "cpuWaitFunctions": ["wait", "rdpq_sync", "rdpq_fence", "__rsp_check_assert"],
  "imem": {
    "rspq_execute_command": "0x080",
    "rspq_fetch_buffer": "0x01C",
    "rspq_load_overlay": "0x0C4",
    "rspq_fetch_buffer_with_ptr": "0x024",
    "RSPQ_Loop": "0x03C"
  },
  "dmem": {
    "cmds_offset": "0x0A0",
    "cmds_size": "0x100",
    "dram_addr_offset": "0x1DC",
    "ovl_table_offset": "0x008",
    "ovl_idmap_offset": "0x048"
  },
  "overlays": [
    {
      "id": 0,
      "name": "RSPQ",
      "commands": {
        "0x00": {"name": "Wait for Cmds."},
        "0x01": {"name": "Write Word", "fmt": "{addr:dmem} = {value:x}", "args": [
          {"name": "addr", "word": 0},
          {"name": "value", "word": 1}
        ]},
        "0x02": {"name": "Jump", "fmt": "-> {target:x}", "args": [
          {"name": "target", "word": 0}
        ]},
        "0x03": {"name": "Call", "fmt": "call {target:x}  slot {slot:x}", "args": [
          {"name": "target", "word": 0},
          {"name": "slot", "word": 1}
        ]},
        "0x04": {"name": "Return", "fmt": "return  slot {slot:x}", "args": [
          {"name": "slot", "word": 0}
        ]},
        "0x05": {"name": "DMA", "fmt": "RDRAM {rdram:x} <-> DMEM {dmem:x}  len {len:x}", "args": [
          {"name": "rdram", "word": 0},
          {"name": "dmem", "word": 1},
          {"name": "len", "word": 2},
          {"name": "flags", "word": 3}
        ]},
        "0x06": {"name": "Write Status", "fmt": "SP_STATUS = {value:x}", "args": [
          {"name": "value", "word": 0}
        ]},
        "0x07": {"name": "Swap Buff.", "fmt": "swap to {to:x}  save {save:x}", "args": [
          {"name": "to", "word": 0},
          {"name": "save", "word": 1},
          {"name": "mask", "word": 2}
        ]},
        "0x08": {"name": "Test Write Status", "fmt": "wait {mask:x} then SP_STATUS = {value:x}", "args": [
          {"name": "value", "word": 0},
          {"name": "mask", "word": 1}
        ]},
        "0x09": {"name": "RDP Set Buff.", "fmt": "RDP buf {start:x}..{end:x}  sentinel {sentinel:x}", "args": [
          {"name": "end", "word": 0},
          {"name": "start", "word": 1},
          {"name": "sentinel", "word": 2}
        ]},
        "0x0A": {"name": "RDP Append Buff.", "fmt": "RDP append -> {end:x}", "args": [
          {"name": "end", "word": 0}
        ]}
      }
    },
    {
      "name": "tiny3d",
      "commands": {
        "0x00": {"name": "Tri Draw", "fmt": "Indices: {v0:x} {v1:x} {v2:x}", "args": [
          {"name": "v0", "word": 0},
          {"name": "v1", "word": 1, "bits": [31, 16]},
          {"name": "v2", "word": 1, "bits": [15, 0]}
        ]},
        "0x01": {"name": "Screen Size", "fmt": "offset: {offX},{offY}, scale: {scaleX},{scaleY}, depth: {depth}, normW: {normW}, guard-band: {guard}", "args": [
          {"name": "offX",   "word": 1, "bits": [31, 17]},
          {"name": "offY",   "word": 1, "bits": [16, 1]},
          {"name": "scaleX", "word": 2, "bits": [31, 16], "signed": true},
          {"name": "scaleY", "word": 2, "bits": [15, 0], "signed": true},
          {"name": "depth",  "word": 3, "bits": [31, 16]},
          {"name": "normW",  "word": 3, "bits": [15, 0], "fdiv": 65535},
          {"name": "guard",  "word": 0, "bits": [3, 0]}
        ]},
        "0x02": {"name": "Matrix Stack", "fmt": "advance {advance}B  mat {mat:x}  (mul {mul} moveOnly {moveOnly})", "args": [
          {"name": "mul", "word": 0, "bits": [0, 0]},
          {"name": "moveOnly", "word": 0, "bits": [1, 1]},
          {"name": "advance", "word": 0, "bits": [23, 8], "signed": true},
          {"name": "mat", "word": 1}
        ]},
        "0x03": {"name": "Set Word", "fmt": "DMEM[{addr:dmem}] = {value:x} ({width})", "args": [
          {"name": "addr", "word": 0, "bits": [14, 0]},
          {"name": "width", "word": 0, "bits": [15, 15], "enum": {"0": "u32", "1": "u16"}},
          {"name": "value", "word": 1}
        ]},
        "0x04": {"name": "Vert Load", "fmt": "load {count} verts  {rdram:x} -> dmem {dst:x}", "args": [
          {"name": "count", "word": 0, "div": 16},
          {"name": "rdram", "word": 1},
          {"name": "dst", "word": 2, "bits": [31, 16]},
          {"name": "src", "word": 2, "bits": [15, 0]}
        ]},
        "0x05": {"name": "Light Set", "fmt": "addr {addr:x}  color {r},{g},{b},{a}", "args": [
          {"name": "addr", "word": 0},
          {"name": "r", "word": 1, "bits": [31, 24]},
          {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]},
          {"name": "a", "word": 1, "bits": [7, 0]},
          {"name": "dir0", "word": 2},
          {"name": "dir1", "word": 3}
        ]},
        "0x06": {"name": "Draw Flags", "fmt": "cull {cull}  rdpCmd {cmd:x}", "args": [
          {"name": "cull", "word": 0, "enum": {"0": "back", "1": "front", "2": "none"}},
          {"name": "cmd", "word": 1}
        ]},
        "0x07": {"name": "Proj Set", "fmt": "proj mat {mat:x}", "args": [
          {"name": "mat", "word": 0}
        ]},
        "0x08": {"name": "Patch", "fmt": "patch {rdram:x} = {op0:x} {op1:x}", "args": [
          {"name": "rdram", "word": 0},
          {"name": "op0", "word": 1},
          {"name": "op1", "word": 2}
        ]},
        "0x09": {"name": "Fog State", "fmt": "fog {val:x}", "args": [
          {"name": "val", "word": 0}
        ]},
        "0x0A": {"name": "Tri Sync", "fmt": "sync"},
        "0x0B": {"name": "Tri Strip", "fmt": "strip {rdram:x}  {count} idx -> dmem {dst:x}", "args": [
          {"name": "rdram", "word": 0},
          {"name": "dst", "word": 1, "bits": [31, 16]},
          {"name": "count", "word": 1, "bits": [15, 0], "add": 1, "div": 2}
        ]},
        "0x0C": {"name": "Tri Seq", "fmt": "seq {v0:x}..{end:x}  {kind}", "args": [
          {"name": "v0", "word": 0},
          {"name": "end", "word": 1, "bits": [15, 0]},
          {"name": "kind", "word": 1, "bits": [31, 16], "enum": {"0": "tris", "36": "quads"}}
        ]}
      }
    },
    {
      "name": "rdpq",
      "commands": {
        "0x00": {"name": "Noop"},
        "0x01": {"name": "Set Lookup Addr"},
        "0x02": {"name": "Fill Rect Ex"},
        "0x03": {"name": "Clear ZBuf"},
        "0x04": {"name": "Mode Standard"},
        "0x05": {"name": "Combine 2-Pass", "fmt": "{rdp:3c}"},
        "0x06": {"name": "Push Mode", "fmt": "Push Render-Mode"},
        "0x07": {"name": "Pop Mode", "fmt": "Pop Render-Mode"},
        "0x08": {"name": "Tri Fill", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]},
          {"name": "ym", "word": 1, "bits": [31, 16]},
          {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x09": {"name": "Tri Fill ZBuf", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x0A": {"name": "Tri Tex", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x0B": {"name": "Tri Tex ZBuf", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x0C": {"name": "Tri Shade", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x0D": {"name": "Tri Shade ZBuf", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x0E": {"name": "Tri Shade Tex", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x0F": {"name": "Tri Shade Tex ZBuf", "fmt": "{edge}  tile {tile}  y [{yl},{ym},{yh}]", "args": [
          {"name": "edge", "word": 0, "bits": [23, 23], "enum": {"0": "right", "1": "left"}},
          {"name": "tile", "word": 0, "bits": [18, 16]},
          {"name": "yl", "word": 0, "bits": [15, 0]}, {"name": "ym", "word": 1, "bits": [31, 16]}, {"name": "yh", "word": 1, "bits": [15, 0]}
        ]},
        "0x10": {"name": "Tex Rect Ex"},
        "0x11": {"name": "Debug Mode"},
        "0x12": {"name": "Scissor Ex"},
        "0x13": {"name": "Prim Color Comp"},
        "0x14": {"name": "Modify SOM"},
        "0x15": {"name": "Reserved"},
        "0x16": {"name": "Fill Color 32", "fmt": "RGBA: {r} {g} {b} {a}", "args": [
          {"name": "r", "word": 1, "bits": [31, 24]}, {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]}, {"name": "a", "word": 1, "bits": [7, 0]}
        ]},
        "0x17": {"name": "Reserved"},
        "0x18": {"name": "Blending Mode"},
        "0x19": {"name": "Fog Mode"},
        "0x1A": {"name": "RDP Wait Idle", "fmt": "Wait for RDP idle"},
        "0x1B": {"name": "Combine 1-Pass","fmt": "{rdp:3c}"},
        "0x1C": {"name": "AutoTMEM Addr"},
        "0x1D": {"name": "AutoTMEM Tile"},
        "0x1E": {"name": "Triangle"},
        "0x1F": {"name": "Triangle Data"},
        "0x24": {"name": "Tex Rect", "fmt": "Tile{tile}, from {ulx},{uly} to {lrx},{lry}", "args": [
          {"name": "tile", "word": 1, "bits": [26, 24]},
          {"name": "ulx", "word": 1, "bits": [23, 12]}, {"name": "uly", "word": 1, "bits": [11, 0]},
          {"name": "lrx", "word": 0, "bits": [23, 12]}, {"name": "lry", "word": 0, "bits": [11, 0]}
        ]},
        "0x25": {"name": "Tex Rect Flip", "fmt": "Tile{tile}, from {ulx},{uly} to {lrx},{lry}", "args": [
          {"name": "tile", "word": 1, "bits": [26, 24]},
          {"name": "ulx", "word": 1, "bits": [23, 12]}, {"name": "uly", "word": 1, "bits": [11, 0]},
          {"name": "lrx", "word": 0, "bits": [23, 12]}, {"name": "lry", "word": 0, "bits": [11, 0]}
        ]},
        "0x26": {"name": "Sync Load", "fmt": "Sync Load (25 stalls)"},
        "0x27": {"name": "Sync Pipe", "fmt": "Sync Pipe (50 stalls)"},
        "0x28": {"name": "Sync Tile", "fmt": "Sync Tile (33 stalls)"},
        "0x29": {"name": "Sync Full", "fmt": "Sync Full"},
        "0x2A": {"name": "Set Key GB", "fmt": "G[w{gw} c{gc} s{gs}]  B[w{bw} c{bc} s{bs}]", "args": [
          {"name": "gw", "word": 0, "bits": [23, 12]}, {"name": "gc", "word": 1, "bits": [23, 16]}, {"name": "gs", "word": 1, "bits": [15, 8]},
          {"name": "bw", "word": 0, "bits": [11, 0]}, {"name": "bc", "word": 1, "bits": [7, 0]}, {"name": "bs", "word": 1, "bits": [31, 24]}
        ]},
        "0x2B": {"name": "Set Key R", "fmt": "R[w{rw} c{rc} s{rs}]", "args": [
          {"name": "rw", "word": 1, "bits": [27, 16]}, {"name": "rc", "word": 1, "bits": [7, 0]}, {"name": "rs", "word": 1, "bits": [15, 8]}
        ]},
        "0x2C": {"name": "Set Convert", "fmt": "{rdp:2c}"},
        "0x2D": {"name": "Set Scissor", "fmt": "min: {ulx},{uly}  max: {lrx},{lry}  field {field}", "args": [
          {"name": "ulx", "word": 0, "bits": [23, 12]}, {"name": "uly", "word": 0, "bits": [11, 0]},
          {"name": "lrx", "word": 1, "bits": [23, 12]}, {"name": "lry", "word": 1, "bits": [11, 0]},
          {"name": "field", "word": 1, "bits": [25, 25]}
        ]},
        "0x2E": {"name": "Set Prim Depth", "fmt": "z={z}  dz={dz}", "args": [
          {"name": "z", "word": 1, "bits": [31, 16]},
          {"name": "dz", "word": 1, "bits": [15, 0]}
        ]},
        "0x2F": {"name": "Set Other Modes", "fmt": "{cycle}  z {zmode}  zUpd {zUpd} zCmp {zCmp} aa {aa} blend {blend} persp {persp} tlut {tlut}", "args": [
          {"name": "cycle", "word": 0, "bits": [21, 20], "enum": {"0": "1cycle", "1": "2cycle", "2": "copy", "3": "fill"}},
          {"name": "persp", "word": 0, "bits": [19, 19]},
          {"name": "tlut",  "word": 0, "bits": [15, 15]},
          {"name": "zmode", "word": 1, "bits": [11, 10], "enum": {"0": "opaque", "1": "interpen", "2": "transp", "3": "decal"}},
          {"name": "zUpd",  "word": 1, "bits": [5, 5]},
          {"name": "zCmp",  "word": 1, "bits": [4, 4]},
          {"name": "aa",    "word": 1, "bits": [3, 3]},
          {"name": "blend", "word": 1, "bits": [14, 14]}
        ]},
        "0x30": {"name": "Load TLUT", "fmt": "tile {tile}  ul {uls},{ult}  lr {lrs},{lrt}", "args": [
          {"name": "tile", "word": 1, "bits": [26, 24]},
          {"name": "uls", "word": 0, "bits": [23, 12]}, {"name": "ult", "word": 0, "bits": [11, 0]},
          {"name": "lrs", "word": 1, "bits": [23, 12]}, {"name": "lrt", "word": 1, "bits": [11, 0]}
        ]},
        "0x31": {"name": "Reserved"},
        "0x32": {"name": "Set Tile Size", "fmt": "Tile{tile} ul {uls},{ult}  lr {lrs},{lrt}", "args": [
          {"name": "tile", "word": 1, "bits": [26, 24]},
          {"name": "uls", "word": 0, "bits": [23, 12]}, {"name": "ult", "word": 0, "bits": [11, 0]},
          {"name": "lrs", "word": 1, "bits": [23, 12]}, {"name": "lrt", "word": 1, "bits": [11, 0]}
        ]},
        "0x33": {"name": "Load Block", "fmt": "Tile{tile} ul {uls},{ult}  lrs {lrs}  dxt {dxt}", "args": [
          {"name": "tile", "word": 1, "bits": [26, 24]},
          {"name": "uls", "word": 0, "bits": [23, 12]}, {"name": "ult", "word": 0, "bits": [11, 0]},
          {"name": "lrs", "word": 1, "bits": [23, 12]}, {"name": "dxt", "word": 1, "bits": [11, 0]}
        ]},
        "0x34": {"name": "Load Tile", "fmt": "Tile{tile} ul {uls},{ult}  lr {lrs},{lrt}", "args": [
          {"name": "tile", "word": 1, "bits": [26, 24]},
          {"name": "uls", "word": 0, "bits": [23, 12]}, {"name": "ult", "word": 0, "bits": [11, 0]},
          {"name": "lrs", "word": 1, "bits": [23, 12]}, {"name": "lrt", "word": 1, "bits": [11, 0]}
        ]},
        "0x35": {"name": "Set Tile", "fmt": "Tile{tile} {format} {size}  line {line}  tmem {tmem:x}  pal {pal}", "args": [
          {"name": "tile",   "word": 1, "bits": [26, 24]},
          {"name": "format", "word": 0, "bits": [23, 21], "enum": {"0": "RGBA", "1": "YUV", "2": "CI", "3": "IA", "4": "I", "5": "I", "6": "I", "7": "I"}},
          {"name": "size",   "word": 0, "bits": [20, 19], "enum": {"0": "4bpp", "1": "8bpp", "2": "16bpp", "3": "32bpp"}},
          {"name": "line",   "word": 0, "bits": [18, 9]},
          {"name": "tmem",   "word": 0, "bits": [8, 0]},
          {"name": "pal",    "word": 1, "bits": [23, 20]}
        ]},
        "0x36": {"name": "Fill Rect", "fmt": "ul {ulx},{uly}  lr {lrx},{lry}", "args": [
          {"name": "ulx", "word": 1, "bits": [23, 12]}, {"name": "uly", "word": 1, "bits": [11, 0]},
          {"name": "lrx", "word": 0, "bits": [23, 12]}, {"name": "lry", "word": 0, "bits": [11, 0]}
        ]},
        "0x37": {"name": "Set Fill Color", "fmt": "RGBA: {r},{g},{b},{a}", "args": [
          {"name": "r", "word": 1, "bits": [31, 24]}, {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]}, {"name": "a", "word": 1, "bits": [7, 0]}
        ]},
        "0x38": {"name": "Set Fog Color", "fmt": "RGBA: {r},{g},{b},{a}", "args": [
          {"name": "r", "word": 1, "bits": [31, 24]}, {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]}, {"name": "a", "word": 1, "bits": [7, 0]}
        ]},
        "0x39": {"name": "Set Blend Color", "fmt": "RGBA: {r},{g},{b},{a}", "args": [
          {"name": "r", "word": 1, "bits": [31, 24]}, {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]}, {"name": "a", "word": 1, "bits": [7, 0]}
        ]},
        "0x3A": {"name": "Set Prim Color", "fmt": "RGBA: {r},{g},{b},{a}", "args": [
          {"name": "r", "word": 1, "bits": [31, 24]}, {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]}, {"name": "a", "word": 1, "bits": [7, 0]}
        ]},
        "0x3B": {"name": "Set Env Color", "fmt": "RGBA: {r},{g},{b},{a}", "args": [
          {"name": "r", "word": 1, "bits": [31, 24]}, {"name": "g", "word": 1, "bits": [23, 16]},
          {"name": "b", "word": 1, "bits": [15, 8]}, {"name": "a", "word": 1, "bits": [7, 0]}
        ]},
        "0x3C": {"name": "Set Combine", "fmt": "{rdp:3c}"},
        "0x3D": {"name": "Set Tex Image", "fmt": "{format}  {width}x{height}  addr {addr:x}", "args": [
          {"name": "format", "word": 0, "bits": [23, 19], "enum": {"0": "NONE", "2": "RGBA16", "3": "RGBA32", "6": "YUV16", "8": "CI4", "9": "CI8", "12": "IA4", "13": "IA8", "14": "IA16", "16": "I4", "17": "I8"}},
          {"name": "width",  "word": 0, "bits": [9, 0], "add": 1},
          {"name": "height", "word": 0, "bits": [18, 10], "add": 1},
          {"name": "addr",   "word": 1, "bits": [24, 0]}
        ]},
        "0x3E": {"name": "Set Depth Image", "fmt": "Address {addr:x}", "args": [
          {"name": "addr", "word": 1, "bits": [24, 0]}
        ]},
        "0x3F": {"name": "Set Color Image", "fmt": "{format}  stride {stride}px  h {height}  Address {addr:x}", "args": [
          {"name": "format", "word": 0, "bits": [23, 19], "enum": {"0": "NONE", "2": "RGBA16", "3": "RGBA32", "6": "YUV16", "8": "CI4", "9": "CI8", "12": "IA4", "13": "IA8", "14": "IA16", "16": "I4", "17": "I8"}},
          {"name": "stride", "word": 0, "bits": [9, 0], "add": 1},
          {"name": "height", "word": 0, "bits": [18, 10], "add": 1},
          {"name": "addr",   "word": 1, "bits": [24, 0]}
        ]}
      }
    },
    {
      "name": "tinypx",
      "commands": {
        "0x00": {"name": "Sync T3D", "fmt": "mat {mat:x}  screen {screen:x}  normW {normW}", "args": [
          {"name": "mat", "word": 0},
          {"name": "screen", "word": 1},
          {"name": "normW", "word": 2, "bits": [15, 0], "fdiv": 65535}
        ]},
        "0x01": {"name": "Draw Color", "fmt": "draw color  size {size}  src {src:x}  {kind}", "args": [
          {"name": "size", "word": 0},
          {"name": "src", "word": 1, "bits": [23, 0]},
          {"name": "kind", "word": 1, "bits": [31, 31], "enum": {"0": "s8", "1": "s16"}}
        ]},
        "0x02": {"name": "Matrix Stack", "fmt": "advance {advance}B  mat {mat:x}  (mul {mul} moveOnly {moveOnly})", "args": [
          {"name": "mul", "word": 0, "bits": [0, 0]},
          {"name": "moveOnly", "word": 0, "bits": [1, 1]},
          {"name": "advance", "word": 0, "bits": [23, 8], "signed": true},
          {"name": "mat", "word": 1}
        ]},
        "0x03": {"name": "Set DMEM", "fmt": "DMEM[{addr:dmem}] = {value:x} ({width})", "args": [
          {"name": "addr", "word": 0, "bits": [14, 0]},
          {"name": "width", "word": 0, "bits": [15, 15], "enum": {"0": "u32", "1": "u16"}},
          {"name": "value", "word": 1}
        ]},
        "0x04": {"name": "Draw Textured", "fmt": "draw textured  size {size}  src {src:x}  {kind}", "args": [
          {"name": "size", "word": 0},
          {"name": "src", "word": 1, "bits": [23, 0]},
          {"name": "kind", "word": 1, "bits": [31, 31], "enum": {"0": "s8", "1": "s16"}}
        ]}
      }
    }
,
    {
      "name": "magma",
      "commands": {
        "0x00": {"name": "Set Byte",   "fmt": "state[{offset:x}] = {value:x}",          "args": [{"name": "offset", "word": 0}, {"name": "value", "word": 1}]},
        "0x01": {"name": "Set Short",  "fmt": "state[{offset:x}] = {value:x}",          "args": [{"name": "offset", "word": 0}, {"name": "value", "word": 1}]},
        "0x02": {"name": "Set Word",   "fmt": "state[{offset:x}] = {value:x}",          "args": [{"name": "offset", "word": 0}, {"name": "value", "word": 1}]},
        "0x03": {"name": "Set Quad",   "fmt": "state[{offset:x}] = {v0:x} {v1:x} {v2:x} {v3:x}", "args": [{"name": "offset", "word": 0}, {"name": "v0", "word": 1}, {"name": "v1", "word": 2}, {"name": "v2", "word": 3}, {"name": "v3", "word": 4}]},
        "0x04": {"name": "Set Shader", "fmt": "code {code:x}  size {size}B",             "args": [{"name": "code", "word": 0}, {"name": "size", "word": 1}]},
        "0x05": {"name": "Load Vertices", "fmt": "buf {buffer}  cache {cache}  count {count}", "args": [{"name": "buffer", "word": 0}, {"name": "cache", "word": 1, "bits": [31, 16]}, {"name": "count", "word": 1, "bits": [15, 0]}]},
        "0x06": {"name": "Draw Indices", "fmt": "tri {i0} {i1} {i2}",                    "args": [{"name": "i0", "word": 0}, {"name": "i1", "word": 1, "bits": [31, 16]}, {"name": "i2", "word": 1, "bits": [15, 0]}]},
        "0x07": {"name": "Draw End"},
        "0x08": {"name": "Load Uniform", "fmt": "addr {addr:x}  off {offset}  size {size}B", "args": [{"name": "addr", "word": 0}, {"name": "size", "word": 1, "bits": [31, 16], "add": 1}, {"name": "offset", "word": 1, "bits": [15, 0]}]},
        "0x09": {"name": "Inline Uniform 8"},
        "0x0A": {"name": "Inline Uniform 16"},
        "0x0B": {"name": "Inline Uniform 32"},
        "0x0C": {"name": "Inline Uniform 64"},
        "0x0D": {"name": "Inline Uniform 128"},
        "0x0E": {"name": "Inline Uniform Max"}
      }
    },
    {
      "name": "gl",
      "commands": {
        "0x00": {"name": "Set Flag"},
        "0x01": {"name": "Set Byte", "fmt": "state[{offset:x}] = {value:x}  update {update}", "args": [{"name": "offset", "word": 0, "bits": [11, 0]}, {"name": "update", "word": 0, "bits": [23, 12]}, {"name": "value", "word": 1}]},
        "0x02": {"name": "Set Short", "fmt": "state[{offset:x}] = {value:x}  update {update}", "args": [{"name": "offset", "word": 0, "bits": [11, 0]}, {"name": "update", "word": 0, "bits": [23, 12]}, {"name": "value", "word": 1}]},
        "0x03": {"name": "Set Word", "fmt": "state[{offset:x}] = {value:x}  update {update}", "args": [{"name": "offset", "word": 0, "bits": [11, 0]}, {"name": "update", "word": 0, "bits": [23, 12]}, {"name": "value", "word": 1}]},
        "0x04": {"name": "Set Long", "fmt": "state[{offset:x}] = {v0:x} {v1:x}  update {update}", "args": [{"name": "offset", "word": 0, "bits": [11, 0]}, {"name": "update", "word": 0, "bits": [23, 12]}, {"name": "v0", "word": 1}, {"name": "v1", "word": 2}]},
        "0x05": {"name": "Update", "fmt": "update {update}", "args": [{"name": "update", "word": 0, "bits": [23, 12]}]},
        "0x06": {"name": "Bind Texture", "fmt": "texture[{tex:x}] = {ptr:x}", "args": [{"name": "tex", "word": 0, "bits": [15, 0]}, {"name": "ptr", "word": 1}]},
        "0x07": {"name": "Get Value", "fmt": "offset {offset:x}  size {size}B  dest {dest:x}", "args": [{"name": "offset", "word": 0, "bits": [11, 0]}, {"name": "size", "word": 0, "bits": [23, 12]}, {"name": "dest", "word": 1}]},
        "0x08": {"name": "Copy Fill Color", "fmt": "offset {offset:x}", "args": [{"name": "offset", "word": 0}]},
        "0x09": {"name": "Set Light Pos", "fmt": "offset {offset:x}  [{x:x}, {y:x}, {z:x}, {w:x}]", "args": [{"name": "offset", "word": 0}, {"name": "x", "word": 1, "bits": [31, 16]}, {"name": "y", "word": 1, "bits": [15, 0]}, {"name": "z", "word": 2, "bits": [31, 16]}, {"name": "w", "word": 2, "bits": [15, 0]}]},
        "0x0A": {"name": "Matrix Push"},
        "0x0B": {"name": "Matrix Pop"},
        "0x0C": {"name": "Matrix Load", "fmt": "multiply {mult}", "args": [{"name": "mult", "word": 0, "bits": [1, 0]}]},
        "0x0D": {"name": "Set Palette Index", "fmt": "index {index:x}", "args": [{"name": "index", "word": 0}]},
        "0x0E": {"name": "Matrix Copy", "fmt": "matrix {mat:x}", "args": [{"name": "mat", "word": 0}]}
      }
    },
    {
      "name": "gl2",
      "commands": {
        "0x00": {"name": "Pre Init Pipe", "fmt": "prim mode {prim}", "args": [{"name": "prim", "word": 0}]},
        "0x01": {"name": "Pre Init Pipe Tex"},
        "0x02": {"name": "Set Pipeline", "fmt": "pipeline[{offset:x}] = code {code:x}  size {size}B", "args": [{"name": "code", "word": 0}, {"name": "offset", "word": 1, "bits": [31, 16]}, {"name": "size", "word": 1, "bits": [15, 0]}]},
        "0x03": {"name": "Pre Init Magma", "fmt": "magma state {state:x}  client flags {flags:x}", "args": [{"name": "state", "word": 0}, {"name": "flags", "word": 1}]}
      }
    }
  ]
}
)ARES_RSPQ"
