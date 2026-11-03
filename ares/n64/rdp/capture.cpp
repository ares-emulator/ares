auto RDP::Debugger::requestCapture() -> void {
  if(!capture) capture = std::make_unique<Capture>();
  capture->ready.store(false, std::memory_order_relaxed);
  capture->armedFields = 0;
  capture->activeFields = 0;
  capture->armed.store(true, std::memory_order_release);
}

auto RDP::Debugger::captureReady() const -> bool {
  return capture && capture->ready.load(std::memory_order_acquire);
}

auto RDP::Debugger::captureActive() const -> bool {
  return capture && capture->active;
}

auto RDP::Debugger::frameCapture() const -> const RDPFrameCapture* {
  return captureReady() ? &capture->frame : nullptr;
}

auto RDP::Debugger::captureCommandCount() const -> u32 {
  if(auto frame = frameCapture()) return frame->commandOffsets.size();
  return 0;
}

//Opcode names, normalized: the eight triangle opcodes and both texture rectangle
//opcodes differ only in which operands follow, which the argument summary states,
//so they share one name and align down the command list as one kind of command.
static const char* commandNameTable[64] = {
  "No Operation", "Invalid 01", "Invalid 02", "Invalid 03",
  "Invalid 04", "Invalid 05", "Invalid 06", "Invalid 07",
  "Triangle", "Triangle", "Triangle", "Triangle",
  "Triangle", "Triangle", "Triangle", "Triangle",
  "Invalid 10", "Invalid 11", "Invalid 12", "Invalid 13",
  "Invalid 14", "Invalid 15", "Invalid 16", "Invalid 17",
  "Invalid 18", "Invalid 19", "Invalid 1a", "Invalid 1b",
  "Invalid 1c", "Invalid 1d", "Invalid 1e", "Invalid 1f",
  "Invalid 20", "Invalid 21", "Invalid 22", "Invalid 23",
  "Texture Rectangle", "Texture Rectangle", "Sync Load", "Sync Pipe",
  "Sync Tile", "Sync Full", "Set Key GB", "Set Key R",
  "Set Convert", "Set Scissor", "Set Primitive Depth", "Set Other Modes",
  "Load TLUT", "Invalid 31", "Set Tile Size", "Load Block",
  "Load Tile", "Set Tile", "Fill Rectangle", "Set Fill Color",
  "Set Fog Color", "Set Blend Color", "Set Primitive Color", "Set Environment Color",
  "Set Combine Mode", "Set Texture Image", "Set Mask Image", "Set Color Image",
};

static const std::vector<string> imageFormatNames = {
  "RGBA", "YUV", "CI", "IA", "I", "?5", "?6", "?7"
};
static const std::vector<string> imageSizeNames = {"4bpp", "8bpp", "16bpp", "32bpp"};
static const std::vector<string> cycleNames = {"1-Cycle", "2-Cycle", "Copy", "Fill"};
static const std::vector<string> zModeNames = {
  "Opaque", "Interpenetrating", "Transparent", "Decal"
};
static const std::vector<string> coverageNames = {"Clamp", "Wrap", "Zap", "Save"};
static const std::vector<string> rgbDitherNames = {
  "Magic Square", "Bayer", "Noise", "None"
};
static const std::vector<string> alphaDitherNames = {
  "Pattern", "Inverse Pattern", "Noise", "None"
};

//Colour combiner input mnemonics; the four operand classes index different tables.
static const std::vector<string> combineSubA = {
  "COMBINED", "TEXEL0", "TEXEL1", "PRIM", "SHADE", "ENV", "1", "NOISE"
};
static const std::vector<string> combineSubB = {
  "COMBINED", "TEXEL0", "TEXEL1", "PRIM", "SHADE", "ENV", "CENTER", "K4"
};
static const std::vector<string> combineMul = {
  "COMBINED", "TEXEL0", "TEXEL1", "PRIM", "SHADE", "ENV", "SCALE", "COMBINED_A",
  "TEXEL0_A", "TEXEL1_A", "PRIM_A", "SHADE_A", "ENV_A", "LOD_FRAC", "PRIM_LOD_FRAC", "K5"
};
static const std::vector<string> combineAdd = {
  "COMBINED", "TEXEL0", "TEXEL1", "PRIM", "SHADE", "ENV", "1", "0"
};
static const std::vector<string> combineAlpha = {
  "COMBINED", "TEXEL0", "TEXEL1", "PRIM", "SHADE", "ENV", "1", "0"
};
static const std::vector<string> combineAlphaMul = {
  "LOD_FRAC", "TEXEL0", "TEXEL1", "PRIM", "SHADE", "ENV", "PRIM_LOD_FRAC", "0"
};

//Blender operands. Each of the four multiplexers holds one selection per cycle,
//the two packed adjacently, cycle 0 in the higher pair of bits.
static const std::vector<string> blendP = {"IN", "MEM", "BLEND", "FOG"};
static const std::vector<string> blendA = {"IN_A", "FOG_A", "SHADE_A", "0"};
static const std::vector<string> blendM = {"IN", "MEM", "BLEND", "FOG"};
static const std::vector<string> blendB = {"1-A", "MEM_A", "1", "0"};

static auto combineName(const std::vector<string>& names, u32 index) -> string {
  return index < names.size() ? names[index] : string("0");
}

//(A - B) * C + D
static auto combineEquation(
  const std::vector<string>& a, u32 ai, const std::vector<string>& b, u32 bi,
  const std::vector<string>& c, u32 ci, const std::vector<string>& d, u32 di
) -> string {
  return {
    "(", combineName(a, ai), " - ", combineName(b, bi), ") * ",
    combineName(c, ci), " + ", combineName(d, di)
  };
}

//P * A + M * B, for one blender cycle.
static auto blendEquation(u64 modes, u32 cycle) -> string {
  return {
    combineName(blendP, modes >> 30 - cycle * 2 & 3), " * ",
    combineName(blendA, modes >> 26 - cycle * 2 & 3), " + ",
    combineName(blendM, modes >> 22 - cycle * 2 & 3), " * ",
    combineName(blendB, modes >> 18 - cycle * 2 & 3)
  };
}

//Set_Other_Modes bits that read as plain on/off switches. Only the set ones are
//named, so the result is short for the modes a game actually uses.
static auto otherModesFlags(u64 modes) -> string {
  string flags;
  auto flag = [&](u32 bit, string_view name) {
    if(modes >> bit & 1) flags.append(flags ? " " : "", name);
  };
  flag(55, "atomic"); flag(51, "persp"); flag(50, "detail"); flag(49, "sharpen");
  flag(48, "lod"); flag(47, "tlut"); flag(46, "tlut_ia"); flag(45, "sample_2x2");
  flag(44, "mid_texel"); flag(43, "bilerp0"); flag(42, "bilerp1");
  flag(41, "convert_one"); flag(40, "key");
  flag(14, "force_blend"); flag(13, "alpha_cvg"); flag(12, "cvg_x_alpha");
  flag( 7, "color_on_cvg"); flag(6, "image_read"); flag(5, "z_update");
  flag( 4, "z_compare"); flag(3, "aa"); flag(2, "z_prim");
  flag( 1, "dither_alpha"); flag(0, "alpha_compare");
  return flags ? flags : string{"none"};
}

//Fixed point formatting. Field widths vary by command: screen coordinates are
//10.2, texture rectangle coordinates 10.5 with 5.10 steps, triangle coefficients
//16.16 and Load_Block's dxt 1.11.
static auto signExtend(u64 value, u32 bits) -> s64 {
  u64 sign = 1ull << bits - 1;
  return (s64)(value & sign * 2 - 1) - (s64)((value & sign) * 2);
}

static auto formatFixed(s64 value, u32 fractionBits, u32 digits) -> string {
  bool negative = value < 0;
  u64 magnitude = negative ? (u64)-value : (u64)value;
  u64 one = 1ull << fractionBits;
  u64 scale = 1;
  for(u32 digit : range(digits)) scale *= 10;
  u64 integer = magnitude >> fractionBits;
  u64 fraction = ((magnitude & one - 1) * scale + (one >> 1)) >> fractionBits;
  if(fraction >= scale) integer++, fraction -= scale;
  string text{negative ? "-" : "", integer};
  if(digits) text.append(".", pad(string{fraction}, digits, '0'));
  return text;
}

//Screen coordinates, as carried by scissor, rectangle and tile commands.
static auto formatFixed102(u32 value) -> string {
  return formatFixed(value & 0xfff, 2, 2);
}

static auto formatRGBA32(u32 color) -> string {
  return {
    "r ", color >> 24 & 255, "  g ", color >> 16 & 255,
    "  b ", color >> 8 & 255, "  a ", color & 255
  };
}

static auto formatRGBA16(u32 pixel) -> string {
  return {
    "r ", pixel >> 11 & 31, "  g ", pixel >> 6 & 31,
    "  b ", pixel >> 1 & 31, "  a ", pixel & 1
  };
}

//A captured command is a run of 32-bit words, two per 64-bit RDP command word:
//word 2n holds bits 63..32 of command word n and word 2n+1 bits 31..0. The field
//positions below match render.cpp's fetch routines, which are the authority on
//the encoding.
struct DecodedCommand {
  string name;       //normalized opcode name
  string arguments;  //one line operand summary, shown beside the name
  string fields;     //label\tvalue pairs, one per line, for the detail pane
};

static auto decodeCommand(const std::vector<u32>& words) -> DecodedCommand {
  DecodedCommand decoded;
  if(words.empty()) return decoded;

  auto word = [&](u32 index) -> u32 { return index < words.size() ? words[index] : 0; };
  auto qword = [&](u32 index) -> u64 {
    return (u64)word(index * 2) << 32 | word(index * 2 + 1);
  };
  //One 16-bit field of a triangle coefficient block: `pair` selects the 64-bit
  //word within the block, `slot` the channel (R/G/B/A, or S/T/W) inside it.
  auto half = [&](u32 base, u32 pair, u32 slot) -> u32 {
    u32 value = word(base + pair * 2 + (slot >> 1));
    return slot & 1 ? value & 0xffff : value >> 16;
  };
  //Coefficients are split across an integer block and a fraction block; a
  //coefficient is the two halves joined into one 16.16 value.
  auto coefficient = [&](u32 base, u32 integerPair, u32 fractionPair, u32 slot) -> s32 {
    return (s32)(half(base, integerPair, slot) << 16 | half(base, fractionPair, slot));
  };

  string arguments;
  auto argument = [&](const string& text) {
    if(text) arguments.append(arguments ? "  " : "", text);
  };
  string fields;
  auto field = [&](const string& label, const string& value) {
    fields.append(label, "\t", value, "\n");
  };

  u64 op = qword(0);
  u32 opcode = op >> 56 & 63;
  decoded.name = commandNameTable[opcode];

  switch(opcode) {

  case 0x08: case 0x09: case 0x0a: case 0x0b:
  case 0x0c: case 0x0d: case 0x0e: case 0x0f: {
    bool zbuffer = opcode & 1, texture = opcode & 2, shade = opcode & 4;
    string kind;
    if(shade) kind.append(kind ? " " : "", "shade");
    if(texture) kind.append(kind ? " " : "", "tex");
    if(zbuffer) kind.append(kind ? " " : "", "zbuf");
    if(!kind) kind = "flat";

    u32 tile = op >> 48 & 7;
    u32 level = op >> 51 & 7;
    bool major = op >> 55 & 1;
    //The three scanline bounds are 11.2; the edges they belong to are 16.16.
    s64 yl = signExtend(op >> 32, 14);
    s64 ym = signExtend(op >> 16, 14);
    s64 yh = signExtend(op >>  0, 14);
    s32 xl = word(2), dxldy = word(3);
    s32 xh = word(4), dxhdy = word(5);
    s32 xm = word(6), dxmdy = word(7);

    argument(kind);
    if(texture) argument({"tile ", tile});
    if(level) argument({"level ", level});
    argument({"y ", formatFixed(yh, 2, 2), " -> ", formatFixed(yl, 2, 2)});
    argument({"x ", formatFixed(xh, 16, 2), " -> ", formatFixed(xl, 16, 2)});

    field("Kind", kind);
    field("Tile", {tile});
    field("Level", {level});
    field("Major Edge", major ? "left" : "right");
    field("Scanlines", {
      "YH ", formatFixed(yh, 2, 2), "   YM ", formatFixed(ym, 2, 2),
      "   YL ", formatFixed(yl, 2, 2)
    });
    field("Edge Hi", {"X ", formatFixed(xh, 16, 4), "   dxdy ", formatFixed(dxhdy, 16, 4)});
    field("Edge Mid", {"X ", formatFixed(xm, 16, 4), "   dxdy ", formatFixed(dxmdy, 16, 4)});
    field("Edge Lo", {"X ", formatFixed(xl, 16, 4), "   dxdy ", formatFixed(dxldy, 16, 4)});

    u32 base = 8;
    if(shade) {
      static const char* channels[4] = {"R", "G", "B", "A"};
      auto block = [&](u32 integerPair, u32 fractionPair) -> string {
        string text;
        for(u32 slot : range(4)) {
          text.append(slot ? "  " : "", channels[slot], " ", pad(
            formatFixed(coefficient(base, integerPair, fractionPair, slot), 16, 2), -9L
          ));
        }
        return text.strip();
      };
      field("Shade", block(0, 2));
      field("Shade dx", block(1, 3));
      field("Shade de", block(4, 6));
      field("Shade dy", block(5, 7));
      base += 16;
    }
    if(texture) {
      static const char* channels[3] = {"S", "T", "W"};
      auto block = [&](u32 integerPair, u32 fractionPair) -> string {
        string text;
        for(u32 slot : range(3)) {
          text.append(slot ? "  " : "", channels[slot], " ", pad(
            formatFixed(coefficient(base, integerPair, fractionPair, slot), 16, 3), -11L
          ));
        }
        return text.strip();
      };
      field("Texture", block(0, 2));
      field("Texture dx", block(1, 3));
      field("Texture de", block(4, 6));
      field("Texture dy", block(5, 7));
      base += 16;
    }
    if(zbuffer) {
      field("Depth", {"Z ", formatFixed((s32)word(base), 16, 4)});
      field("Depth dx", formatFixed((s32)word(base + 1), 16, 4));
      field("Depth de", formatFixed((s32)word(base + 2), 16, 4));
      field("Depth dy", formatFixed((s32)word(base + 3), 16, 4));
    }
  } break;

  case 0x24: case 0x25:    //Texture_Rectangle, Texture_Rectangle_Flip
  case 0x36: {             //Fill_Rectangle
    bool textured = opcode != 0x36;
    //The RDP names the lower right corner "lo" and the upper left "hi"; they are
    //printed here in the order they read on screen.
    u32 xl = op >> 44 & 0xfff, yl = op >> 32 & 0xfff;
    u32 tile = op >> 24 & 7;
    u32 xh = op >> 12 & 0xfff, yh = op >> 0 & 0xfff;
    string rectangle{
      formatFixed102(xh), ",", formatFixed102(yh), " -> ",
      formatFixed102(xl), ",", formatFixed102(yl)
    };

    if(opcode == 0x25) argument("flip");
    if(textured) argument({"tile ", tile});
    argument(rectangle);
    field("Rectangle", rectangle);
    field("Size", {
      formatFixed((s32)xl - (s32)xh, 2, 2), " x ",
      formatFixed((s32)yl - (s32)yh, 2, 2), " pixels"
    });
    if(textured) {
      s64 s = signExtend(word(2) >> 16, 16), t = signExtend(word(2), 16);
      s64 dsdx = signExtend(word(3) >> 16, 16), dtdy = signExtend(word(3), 16);
      argument({"st ", formatFixed(s, 5, 2), ",", formatFixed(t, 5, 2)});
      argument({"d ", formatFixed(dsdx, 10, 3), "/", formatFixed(dtdy, 10, 3)});
      field("Tile", {tile});
      field("Texture", {"S ", formatFixed(s, 5, 3), "   T ", formatFixed(t, 5, 3)});
      field("Texture Step", {
        "dsdx ", formatFixed(dsdx, 10, 4), "   dtdy ", formatFixed(dtdy, 10, 4)
      });
      if(opcode == 0x25) field("Orientation", "flipped: S runs down, T across");
    }
  } break;

  case 0x26: field("Effect", "stalls until pending texture loads have landed"); break;
  case 0x27: field("Effect", "stalls until the pipeline has drained"); break;
  case 0x28: field("Effect", "stalls until pending tile writes have landed"); break;
  case 0x29: field("Effect", "ends the command list and raises the DP interrupt"); break;

  case 0x2a: {  //Set_Key_GB
    field("Green", {
      "width ", op >> 44 & 0xfff, "   center ", op >> 24 & 255, "   scale ", op >> 16 & 255
    });
    field("Blue", {
      "width ", op >> 32 & 0xfff, "   center ", op >> 8 & 255, "   scale ", op >> 0 & 255
    });
    argument({"g ", op >> 24 & 255, "/", op >> 16 & 255, "  b ", op >> 8 & 255, "/", op & 255});
  } break;

  case 0x2b: {  //Set_Key_R
    field("Red", {
      "width ", op >> 16 & 0xfff, "   center ", op >> 8 & 255, "   scale ", op >> 0 & 255
    });
    argument({"r ", op >> 8 & 255, "/", op & 255});
  } break;

  case 0x2c: {  //Set_Convert
    //Six 9-bit signed YUV coefficients packed end to end.
    string text;
    for(u32 index : range(6)) {
      text.append(index ? "  " : "", "K", index, " ",
        signExtend(op >> 45 - index * 9, 9));
    }
    argument(text);
    field("Coefficients", text);
  } break;

  case 0x2d: {  //Set_Scissor
    string scissor{
      formatFixed102(op >> 44), ",", formatFixed102(op >> 32), " -> ",
      formatFixed102(op >> 12), ",", formatFixed102(op >> 0)
    };
    argument(scissor);
    field("Scissor", scissor);
    if(op >> 25 & 1) {
      argument({"field ", op >> 24 & 1 ? "odd" : "even"});
      field("Interlace", {"keep ", op >> 24 & 1 ? "odd" : "even", " lines"});
    }
  } break;

  case 0x2e: {  //Set_Prim_Depth
    argument({"z ", op >> 16 & 0xffff, "  dz ", op & 0xffff});
    field("Depth", {op >> 16 & 0xffff});
    field("Delta", {op & 0xffff});
  } break;

  case 0x2f: {  //Set_Other_Modes
    argument(cycleNames[op >> 52 & 3]);
    argument(otherModesFlags(op));
    field("Cycle Type", cycleNames[op >> 52 & 3]);
    field("Z Mode", zModeNames[op >> 10 & 3]);
    field("Coverage", coverageNames[op >> 8 & 3]);
    field("RGB Dither", rgbDitherNames[op >> 38 & 3]);
    field("Alpha Dither", alphaDitherNames[op >> 36 & 3]);
    field("Blender 0", blendEquation(op, 0));
    field("Blender 1", blendEquation(op, 1));
    field("Flags", otherModesFlags(op));
  } break;

  case 0x30:    //Load_TLUT
  case 0x32:    //Set_Tile_Size
  case 0x34: {  //Load_Tile
    u32 tile = op >> 24 & 7;
    //Load_TLUT counts palette entries rather than addressing texels, but shares
    //the encoding: its bounds are entry indices scaled by four.
    string bounds{
      formatFixed102(op >> 44), ",", formatFixed102(op >> 32), " -> ",
      formatFixed102(op >> 12), ",", formatFixed102(op >> 0)
    };
    argument({"tile ", tile});
    argument(bounds);
    field("Tile", {tile});
    field(opcode == 0x30 ? "Entries" : "Texels", bounds);
    if(opcode == 0x30) {
      field("Palette Range", {
        (op >> 44 & 0xfff) >> 2, " -> ", (op >> 12 & 0xfff) >> 2
      });
    }
  } break;

  case 0x33: {  //Load_Block
    u32 tile = op >> 24 & 7;
    u32 texels = (op >> 12 & 0xfff) + 1;
    argument({"tile ", tile});
    argument({"s ", op >> 44 & 0xfff, "  t ", op >> 32 & 0xfff});
    argument({texels, " texels"});
    argument({"dxt ", formatFixed(op & 0xfff, 11, 4)});
    field("Tile", {tile});
    field("Origin", {"S ", op >> 44 & 0xfff, "   T ", op >> 32 & 0xfff});
    field("Texels", {texels});
    //dxt is the TMEM row advance per texel; zero loads one continuous row.
    field("Row Step", {formatFixed(op & 0xfff, 11, 6), " (dxt)"});
  } break;

  case 0x35: {  //Set_Tile
    u32 tile = op >> 24 & 7;
    u32 format = op >> 53 & 7, size = op >> 51 & 3;
    string clamp;
    clamp.append(op >> 9 & 1 ? "C" : op >> 8 & 1 ? "M" : "-");
    clamp.append(op >> 19 & 1 ? "C" : op >> 18 & 1 ? "M" : "-");

    argument({"tile ", tile});
    argument({imageFormatNames[format], " ", imageSizeNames[size]});
    argument({"TMEM ", hex((op >> 32 & 0x1ff) * 8, 4L)});
    argument({"line ", (op >> 41 & 0x1ff) * 8});
    if(op >> 20 & 15) argument({"pal ", op >> 20 & 15});
    if(clamp != "--") argument({"cm ", clamp});

    field("Tile", {tile});
    field("Format", {imageFormatNames[format], " ", imageSizeNames[size]});
    field("TMEM Address", hex((op >> 32 & 0x1ff) * 8, 4L));
    field("Line Width", {(op >> 41 & 0x1ff) * 8, " bytes"});
    field("Palette", {op >> 20 & 15});
    field("Mask", {"S ", op >> 4 & 15, "   T ", op >> 14 & 15});
    field("Shift", {"S ", op >> 0 & 15, "   T ", op >> 10 & 15});
    field("Clamp / Mirror", {
      "S ", op >> 9 & 1 ? "clamp" : op >> 8 & 1 ? "mirror" : "wrap",
      "   T ", op >> 19 & 1 ? "clamp" : op >> 18 & 1 ? "mirror" : "wrap"
    });
  } break;

  case 0x37: {  //Set_Fill_Color
    //In fill mode the register is written to the framebuffer verbatim, so at
    //16bpp it is a pair of pixels and at 32bpp a single RGBA colour.
    u32 color = op & 0xffff'ffff;
    argument(hex(color, 8L));
    field("Raw", hex(color, 8L));
    field("As RGBA32", formatRGBA32(color));
    if((color >> 16) == (color & 0xffff)) {
      field("As RGBA16", formatRGBA16(color & 0xffff));
      argument({"rgba16 x2 ", formatRGBA16(color & 0xffff)});
    } else {
      field("As RGBA16", {
        "even ", formatRGBA16(color >> 16), "   odd ", formatRGBA16(color & 0xffff)
      });
    }
  } break;

  case 0x38:    //Set_Fog_Color
  case 0x39:    //Set_Blend_Color
  case 0x3a:    //Set_Prim_Color
  case 0x3b: {  //Set_Env_Color
    u32 color = op & 0xffff'ffff;
    argument(formatRGBA32(color));
    field("Color", formatRGBA32(color));
    field("Raw", hex(color, 8L));
    if(opcode == 0x3a) {
      argument({"lod ", op >> 40 & 31, "/", op >> 32 & 255});
      field("LOD", {
        "minimum ", op >> 40 & 31, "   fraction ", op >> 32 & 255
      });
    }
  } break;

  case 0x3c: {  //Set_Combine
    string rgb0 = combineEquation(
      combineSubA, op >> 52 & 15, combineSubB, op >> 28 & 15,
      combineMul,  op >> 47 & 31, combineAdd,  op >> 15 & 7);
    argument(rgb0);
    field("RGB 0", rgb0);
    field("Alpha 0", combineEquation(
      combineAlpha, op >> 44 & 7, combineAlpha,    op >> 12 & 7,
      combineAlphaMul, op >> 41 & 7, combineAlpha, op >>  9 & 7));
    field("RGB 1", combineEquation(
      combineSubA, op >> 37 & 15, combineSubB, op >> 24 & 15,
      combineMul,  op >> 32 & 31, combineAdd,  op >>  6 & 7));
    field("Alpha 1", combineEquation(
      combineAlpha, op >> 21 & 7, combineAlpha,    op >>  3 & 7,
      combineAlphaMul, op >> 18 & 7, combineAlpha, op >>  0 & 7));
  } break;

  case 0x3d:    //Set_Texture_Image
  case 0x3f: {  //Set_Color_Image
    u32 format = op >> 53 & 7, size = op >> 51 & 3;
    u32 width = (op >> 32 & 1023) + 1;
    u32 address = op & 0x03ff'ffff;
    argument(hex(address, 8L));
    argument({imageFormatNames[format], " ", imageSizeNames[size]});
    argument({"width ", width});
    field("Address", hex(address, 8L));
    field("Format", {imageFormatNames[format], " ", imageSizeNames[size]});
    field("Width", {width, " pixels"});
  } break;

  case 0x3e: {  //Set_Mask_Image
    u32 address = op & 0x03ff'ffff;
    argument(hex(address, 8L));
    field("Address", hex(address, 8L));
    field("Note", "depth buffer; shape follows the color image");
  } break;

  }

  decoded.arguments = arguments;
  decoded.fields = fields;
  return decoded;
}

auto RDP::Debugger::capturedCommandWords(u32 index) const -> const std::vector<u32>* {
  auto frame = frameCapture();
  if(!frame || index >= frame->commandOffsets.size()) return nullptr;
  auto& words = frame->packets[frame->commandOffsets[index]].words;
  return words.empty() ? nullptr : &words;
}

auto RDP::Debugger::captureCommandText(u32 index) const -> string {
  auto words = capturedCommandWords(index);
  if(!words) return {};
  return decodeCommand(*words).name;
}

auto RDP::Debugger::captureCommandArguments(u32 index) const -> string {
  auto words = capturedCommandWords(index);
  if(!words) return {};
  return decodeCommand(*words).arguments;
}

auto RDP::Debugger::captureCommandOpcode(u32 index) const -> u32 {
  auto words = capturedCommandWords(index);
  return words ? (*words)[0] >> 24 & 63 : 0;
}

auto RDP::Debugger::captureCommandType(u32 index) const -> u32 {
  auto frame = frameCapture();
  if(!frame || index >= frame->commandOffsets.size()) return 0;
  auto& words = frame->packets[frame->commandOffsets[index]].words;
  if(words.empty()) return 0;
  u32 opcode = words[0] >> 24 & 63;
  if(opcode >= 0x08 && opcode <= 0x0f) return 1;  //triangle
  if(opcode == 0x24 || opcode == 0x25 || opcode == 0x36) return 1;  //rectangle
  if(opcode == 0x30 || (opcode >= 0x32 && opcode <= 0x35)) return 2;  //load
  if(opcode >= 0x26 && opcode <= 0x29) return 3;  //sync
  return 4;  //state
}

auto RDP::Debugger::captureCommandDetail(u32 index) const -> string {
  auto words = capturedCommandWords(index);
  if(!words) return {};
  auto decoded = decodeCommand(*words);

  string output{
    decoded.name, "   opcode 0x", hex((*words)[0] >> 24 & 63, 2L),
    "   ", words->size(), " words\n\n"
  };
  for(auto& line : nall::split(decoded.fields, "\n")) {
    if(!line) continue;
    auto pair = nall::split(line, "\t", 1L);
    output.append(pad(pair[0], -18L), pair.size() > 1 ? pair[1] : string{}, "\n");
  }

  //The raw words stay available below the decode, one 64-bit command word per
  //line, which is how the encoding is documented.
  output.append("\n");
  for(u32 offset = 0; offset < words->size(); offset += 2) {
    output.append(hex(offset * 4, 4L), "  ", hex((*words)[offset], 8L), " ",
      offset + 1 < words->size() ? hex((*words)[offset + 1], 8L) : string{}, "\n");
  }
  return output;
}

auto RDP::Debugger::captureSummary() const -> string {
  auto frame = frameCapture();
  if(!frame) return "No completed capture.";

  u32 diffs = 0;
  u64 diffBytes = 0;
  u32 viWrites = 0;
  u32 scanouts = 0;
  for(auto& packet : frame->packets) {
    if(packet.type == RDPFrameCapture::Packet::Type::DramDiff) {
      diffs++;
      diffBytes += packet.bytes.size();
    }
    if(packet.type == RDPFrameCapture::Packet::Type::ViRegister) viWrites++;
    if(packet.type == RDPFrameCapture::Packet::Type::Scanout) scanouts++;
  }

  return {
    "Commands: ", frame->commandOffsets.size(),
    "   Packets: ", frame->packets.size(),
    "   DRAM diffs: ", diffs, " (", diffBytes / 1_KiB, " KiB)",
    "   VI writes: ", viWrites,
    "   Scanouts: ", scanouts
  };
}

auto RDP::Debugger::captureViews() const -> std::vector<string> {
  #if defined(VULKAN)
  if(vulkan.enable) return {"Color", "Depth", "Coverage", "Tiles"};
  #endif
  return {};
}

//Decodes one tile descriptor's texels out of the replayed TMEM. Two separate address
//transforms apply, both mirroring parallel-rdp/shaders/texture.h:
//  * odd texel rows are stored with their two 32-bit halves swapped (byte offset ^ 4),
//  * parallel-RDP keeps TMEM as native-endian 16-bit words at swizzled indices, so a
//    byte view of the buffer needs (byte offset ^ 3) to read TMEM's big-endian bytes.
//Both are folded into readByte, so readHalf composes big-endian halfwords as usual.
auto RDP::Debugger::renderCaptureTile(
  const u8* tmem, const RDPCaptureState& state, u32 tileIndex
) const -> Core::Debugger::GraphicsFrame::Image {
  Core::Debugger::GraphicsFrame::Image image;
  auto& tile = state.tiles[tileIndex];
  if(!tile.set || !tmem) return image;

  u32 width  = tile.s1 >= tile.s0 ? ((tile.s1 - tile.s0) >> 2) + 1 : 0;
  u32 height = tile.t1 >= tile.t0 ? ((tile.t1 - tile.t0) >> 2) + 1 : 0;
  if(width <= 1 && height <= 1) return image;  //no Set_Tile_Size seen
  width = min(width, 512u);
  height = min(height, 512u);

  u32 base = tile.address * 8;
  u32 stride = tile.line * 8;
  bool tlutIA = state.otherModes >> 46 & 1;

  //RGBA32 splits its texels across both halves of TMEM and TLUT'd formats keep the
  //palette in the high half, so in both cases texel addresses wrap at 0x7ff.
  u32 addressMask = tile.size == 3 || tile.format == 2 ? 0x7ff : 0xfff;

  auto readByte = [&](u32 offset) -> u8 { return tmem[(offset & 0xfff) ^ 3]; };
  auto readHalf = [&](u32 offset) -> u16 {
    return readByte(offset) << 8 | readByte(offset + 1);
  };
  auto fromRGBA16 = [](u16 pixel) -> u32 {
    return rdpDecodeRGBA16(pixel);
  };
  auto fromGray = [](u8 intensity) -> u32 {
    return rdpDecodeGray(intensity);
  };
  //TLUT entries live in the high half of TMEM, each replicated across 8 bytes.
  auto fromTLUT = [&](u32 index) -> u32 {
    u16 entry = readHalf(0x800 + index * 8);
    return tlutIA ? fromGray(entry >> 8) : fromRGBA16(entry);
  };

  image.width = width;
  image.height = height;
  image.pixels.resize(width * height, 0xff000000);

  for(u32 y : range(height)) {
    u32 row = base + y * stride;
    u32 swap = y & 1 ? 4 : 0;
    for(u32 x : range(width)) {
      u32 color = 0xff000000;
      u32 wide = ((row + x * 2) & addressMask) ^ swap;
      u32 narrow = ((row + x) & addressMask) ^ swap;
      if(tile.size == 3 && tile.format == 0) {          //RGBA32: RG low, BA high
        u16 rg = readHalf(wide);
        u16 ba = readHalf(wide + 0x800);
        color = 0xff000000 | (rg >> 8) << 16 | (rg & 0xff) << 8 | (ba >> 8);
      } else if(tile.size == 2 && tile.format == 0) {   //RGBA16
        color = fromRGBA16(readHalf(wide));
      } else if(tile.size == 2 && tile.format == 3) {   //IA16
        color = fromGray(readHalf(wide) >> 8);
      } else if(tile.size == 1 && tile.format == 2) {   //CI8
        color = fromTLUT(readByte(narrow));
      } else if(tile.size == 1 && tile.format == 3) {   //IA8
        color = fromGray((readByte(narrow) >> 4) * 17);
      } else if(tile.size == 1 && tile.format == 4) {   //I8
        color = fromGray(readByte(narrow));
      } else if(tile.size == 0) {                       //4bpp
        u8 byte = readByte(((row + x / 2) & addressMask) ^ swap);
        u8 nibble = x & 1 ? byte & 15 : byte >> 4;
        if(tile.format == 2) color = fromTLUT(tile.palette << 4 | nibble);
        else if(tile.format == 3) color = fromGray((nibble >> 1) * 36);
        else color = fromGray(nibble * 17);
      }
      image.pixels[y * width + x] = color;
    }
  }
  return image;
}

//Latches one state-setting command. words[0] holds command bits 63..32 and
//words[1] bits 31..0, so a command bit N >= 32 is words[0] bit N-32.
static auto applyStateCommand(RDPCaptureState& state, const u32* words, u32 wordCount) -> void {
  if(wordCount < 2) return;
  u32 opcode = words[0] >> 24 & 63;
  u32 hi = words[0], lo = words[1];

  switch(opcode) {
  case 0x2a:  //Set_Key_GB
    state.keyGB = lo;
    break;
  case 0x2b:  //Set_Key_R
    state.keyR = lo;
    break;
  case 0x2c:  //Set_Convert
    state.convert = (u64)hi << 32 | lo;
    break;
  case 0x2d:  //Set_Scissor
    state.scissorX0 = hi >> 12 & 0xfff;
    state.scissorY0 = hi >>  0 & 0xfff;
    state.scissorField = lo >> 25 & 1;
    state.scissorOdd = lo >> 24 & 1;
    state.scissorX1 = lo >> 12 & 0xfff;
    state.scissorY1 = lo >>  0 & 0xfff;
    break;
  case 0x2e:  //Set_Prim_Depth
    state.primDepthZ = lo >> 16 & 0xffff;
    state.primDepthDZ = lo >> 0 & 0xffff;
    break;
  case 0x2f:  //Set_Other_Modes
    state.otherModes = (u64)hi << 32 | lo;
    break;
  case 0x32: {  //Set_Tile_Size
    auto& tile = state.tiles[lo >> 24 & 7];
    tile.s0 = hi >> 12 & 0xfff;
    tile.t0 = hi >>  0 & 0xfff;
    tile.s1 = lo >> 12 & 0xfff;
    tile.t1 = lo >>  0 & 0xfff;
    tile.set = true;
    break;
  }
  case 0x35: {  //Set_Tile
    auto& tile = state.tiles[lo >> 24 & 7];
    tile.format = hi >> 21 & 7;
    tile.size = hi >> 19 & 3;
    tile.line = hi >> 9 & 0x1ff;
    tile.address = hi >> 0 & 0x1ff;
    tile.palette = lo >> 20 & 15;
    tile.clampT = lo >> 19 & 1;
    tile.mirrorT = lo >> 18 & 1;
    tile.maskT = lo >> 14 & 15;
    tile.shiftT = lo >> 10 & 15;
    tile.clampS = lo >> 9 & 1;
    tile.mirrorS = lo >> 8 & 1;
    tile.maskS = lo >> 4 & 15;
    tile.shiftS = lo >> 0 & 15;
    tile.set = true;
    break;
  }
  case 0x37:  //Set_Fill_Color
    state.fillColor = lo;
    break;
  case 0x38:  //Set_Fog_Color
    state.fogColor = lo;
    break;
  case 0x39:  //Set_Blend_Color
    state.blendColor = lo;
    break;
  case 0x3a:  //Set_Prim_Color
    state.primMinLevel = hi >> 8 & 31;
    state.primLevelFraction = hi >> 0 & 0xff;
    state.primColor = lo;
    break;
  case 0x3b:  //Set_Env_Color
    state.envColor = lo;
    break;
  case 0x3c:  //Set_Combine
    state.combine = (u64)hi << 32 | lo;
    break;
  case 0x3d:  //Set_Texture_Image
    state.textureFormat = hi >> 21 & 7;
    state.textureSize = hi >> 19 & 3;
    state.textureWidth = (hi & 1023) + 1;
    state.textureAddress = lo & 0x03ff'ffff;
    break;
  case 0x3e:  //Set_Mask_Image
    state.depthAddress = lo & 0x03ff'ffff;
    break;
  case 0x3f:  //Set_Color_Image
    state.colorFormat = hi >> 21 & 7;
    state.colorSize = hi >> 19 & 3;
    state.colorWidth = (hi & 1023) + 1;
    state.colorAddress = lo & 0x03ff'ffff;
    break;
  }
}

auto RDP::Debugger::captureStateAt(u32 command) const -> RDPCaptureState {
  RDPCaptureState state;
  auto frame = frameCapture();
  if(!frame) return state;

  for(auto& words : frame->initialCommands) {
    applyStateCommand(state, words.data(), words.size());
  }

  u32 commandIndex = 0;
  for(auto& packet : frame->packets) {
    if(packet.type != RDPFrameCapture::Packet::Type::Commands) continue;
    applyStateCommand(state, packet.words.data(), packet.words.size());
    if(commandIndex++ == command) break;
  }
  return state;
}

auto RDP::Debugger::captureStateText(u32 command) const -> string {
  auto frame = frameCapture();
  if(!frame) return "No completed capture.";
  auto state = captureStateAt(command);
  auto modes = state.otherModes;
  auto combine = state.combine;

  //One field/value pair per line, tab separated; the frontend splits this into a
  //two column table.
  string text;
  auto row = [&](const string& name, const string& value) {
    text.append(name, "\t", value, "\n");
  };

  string scissor{
    formatFixed102(state.scissorX0), ", ", formatFixed102(state.scissorY0), "  ->  ",
    formatFixed102(state.scissorX1), ", ", formatFixed102(state.scissorY1)
  };
  if(state.scissorField) scissor.append("  field ", state.scissorOdd ? "odd" : "even");

  row("Color Image", {hex(state.colorAddress, 8L), "  ",
    imageFormatNames[state.colorFormat], " ", imageSizeNames[state.colorSize],
    "  width ", state.colorWidth});
  row("Depth Image", hex(state.depthAddress, 8L));
  row("Texture Image", {hex(state.textureAddress, 8L), "  ",
    imageFormatNames[state.textureFormat], " ", imageSizeNames[state.textureSize],
    "  width ", state.textureWidth});
  row("Scissor", scissor);

  row("Cycle Type", cycleNames[modes >> 52 & 3]);
  row("Z Mode", zModeNames[modes >> 10 & 3]);
  row("Coverage", coverageNames[modes >> 8 & 3]);
  row("Blender 0", blendEquation(modes, 0));
  row("Blender 1", blendEquation(modes, 1));
  row("Other Modes", hex(modes & 0x00ff'ffff'ffff'ffffull, 14L));
  row("Flags", otherModesFlags(modes));

  row("Combine RGB 0", combineEquation(
    combineSubA, combine >> 52 & 15, combineSubB, combine >> 28 & 15,
    combineMul,  combine >> 47 & 31, combineAdd,  combine >> 15 & 7));
  row("Combine Alpha 0", combineEquation(
    combineAlpha, combine >> 44 & 7, combineAlpha,    combine >> 12 & 7,
    combineAlphaMul, combine >> 41 & 7, combineAlpha, combine >>  9 & 7));
  row("Combine RGB 1", combineEquation(
    combineSubA, combine >> 37 & 15, combineSubB, combine >> 24 & 15,
    combineMul,  combine >> 32 & 31, combineAdd,  combine >>  6 & 7));
  row("Combine Alpha 1", combineEquation(
    combineAlpha, combine >> 21 & 7, combineAlpha,    combine >>  3 & 7,
    combineAlphaMul, combine >> 18 & 7, combineAlpha, combine >>  0 & 7));

  row("Fill Color", hex(state.fillColor, 8L));
  row("Fog Color", hex(state.fogColor, 8L));
  row("Blend Color", hex(state.blendColor, 8L));
  row("Primitive Color", {hex(state.primColor, 8L),
    "   LOD ", state.primMinLevel, "/", state.primLevelFraction});
  row("Environment Color", hex(state.envColor, 8L));
  row("Primitive Depth", {"z ", state.primDepthZ, "  dz ", state.primDepthDZ});
  row("Key R / GB", {hex(state.keyR, 8L), " / ", hex(state.keyGB, 8L)});
  row("Convert", hex(state.convert & 0x00ff'ffff'ffff'ffffull, 14L));

  for(u32 index : range(8)) {
    auto& tile = state.tiles[index];
    if(!tile.set) continue;
    string clamp;
    clamp.append(tile.clampS ? "C" : tile.mirrorS ? "M" : "-");
    clamp.append(tile.clampT ? "C" : tile.mirrorT ? "M" : "-");
    row({"Tile ", index}, {
      pad(string{imageFormatNames[tile.format], " ", imageSizeNames[tile.size]}, -11L),
      "  TMEM ", hex(tile.address * 8, 4L),
      "  line ", pad(string{tile.line * 8}, -4L),
      "  pal ", tile.palette,
      "  mask ", tile.maskS, "/", tile.maskT,
      "  shift ", tile.shiftS, "/", tile.shiftT,
      "  cm ", clamp,
      "  ", formatFixed102(tile.s0), ", ", formatFixed102(tile.t0),
      " -> ", formatFixed102(tile.s1), ", ", formatFixed102(tile.t1)
    });
  }

  #if defined(VULKAN)
  if(auto error = vulkan.replayCrashed()) row("Replay RDP Crash", error);
  #endif
  return text;
}

static auto hashBytes(u64 hash, const void* data, u64 size) -> u64 {
  auto bytes = (const u8*)data;
  for(u64 offset = 0; offset < size; offset++) {
    hash = (hash ^ bytes[offset]) * 0x0000'0100'0000'01b3ull;  //FNV-1a
  }
  return hash;
}

//Games emit a sync around nearly every state change, so most syncs in a capture
//see the same image as the one before them. A scrollback entry is only useful
//where something observable changed, which is decided by hashing the color,
//depth and coverage buffers as they stand at each sync: the capture is replayed
//once from beginning to end and a sync is kept when its hash differs from the
//last kept one. Replaying to every sync in turn costs one GPU flush per sync, so
//the result is cached for the lifetime of the capture.
auto RDP::Debugger::captureTicks() -> std::vector<u32> {
  auto frame = frameCapture();
  if(!frame) return {};
  if(tickIdentifier == frame->identifier) return tickCommands;
  tickIdentifier = frame->identifier;
  tickCommands.clear();

  #if defined(VULKAN)
  if(!vulkan.enable) return tickCommands;

  RDPCaptureState state;
  for(auto& words : frame->initialCommands) {
    applyStateCommand(state, words.data(), words.size());
  }

  u64 previousHash = 0;
  bool havePrevious = false;
  u32 commandIndex = 0;
  for(auto& packet : frame->packets) {
    if(packet.type != RDPFrameCapture::Packet::Type::Commands) continue;
    u32 index = commandIndex++;
    if(packet.words.empty()) continue;
    applyStateCommand(state, packet.words.data(), packet.words.size());

    u32 opcode = packet.words[0] >> 24 & 63;
    if(opcode < 0x26 || opcode > 0x29) continue;  //Sync Load/Pipe/Tile/Full
    if(!vulkan.replay(*frame, index)) continue;
    auto memory = vulkan.replayRdram();
    if(!memory) continue;
    auto hidden = vulkan.replayHiddenRdram();

    u32 width = max(1u, state.colorWidth);
    u32 height = min(480u, (state.scissorY1 > state.scissorY0
      ? state.scissorY1 - state.scissorY0 + 3 : 4) >> 2);
    u32 bytesPerPixel = state.colorSize == 3 ? 4 : state.colorSize == 2 ? 2 : 1;

    //The buffer descriptions are hashed alongside their contents so that
    //retargeting to an identically shaped buffer still counts as a change.
    u32 description[7] = {
      state.colorAddress, state.colorFormat, state.colorSize,
      width, height, state.depthAddress, (u32)(state.otherModes >> 8 & 3)
    };
    u64 hash = hashBytes(0xcbf2'9ce4'8422'2325ull, description, sizeof(description));

    auto hashRegion = [&](u64 address, u64 size, const u8* data, u64 dataSize) {
      if(!data || address >= dataSize) return;
      hash = hashBytes(hash, data + address, min(size, dataSize - address));
    };
    u64 pixels = (u64)width * height;
    hashRegion(state.colorAddress, pixels * bytesPerPixel, memory->data, memory->size);
    hashRegion(state.depthAddress, pixels * 2, memory->data, memory->size);
    hashRegion(state.colorAddress / 2, pixels * bytesPerPixel / 2, hidden, memory->size / 2);

    if(havePrevious && hash == previousHash) continue;
    previousHash = hash;
    havePrevious = true;
    tickCommands.push_back(index);
  }
  #endif
  return tickCommands;
}

auto RDP::Debugger::renderCapture(
  u32 command, u32 view
) -> Core::Debugger::GraphicsFrame::Image {
  Core::Debugger::GraphicsFrame::Image image;
  auto frame = frameCapture();
  #if !defined(VULKAN)
  return image;
  #else
  if(!frame || !vulkan.replay(*frame, command)) return image;

  auto state = captureStateAt(command);

  //All eight tile descriptors composited into one image on a fixed 4x2 grid, so a
  //tile's position identifies it without needing per-cell labels.
  if(view == 3) {
    auto tmem = vulkan.replayTMEM();
    if(!tmem) return image;

    Core::Debugger::GraphicsFrame::Image tiles[8];
    u32 cellWidth = 0, cellHeight = 0;
    for(u32 index : range(8)) {
      tiles[index] = renderCaptureTile(tmem, state, index);
      cellWidth = max(cellWidth, tiles[index].width);
      cellHeight = max(cellHeight, tiles[index].height);
    }
    if(!cellWidth || !cellHeight) return image;

    static constexpr u32 columns = 4, rows = 2;
    image.width = columns * (cellWidth + 1) + 1;
    image.height = rows * (cellHeight + 1) + 1;
    image.pixels.resize(image.width * image.height, 0xff303030);  //gutters

    for(u32 index : range(8)) {
      u32 originX = (index % columns) * (cellWidth + 1) + 1;
      u32 originY = (index / columns) * (cellHeight + 1) + 1;
      for(u32 y : range(cellHeight)) {
        for(u32 x : range(cellWidth)) {
          bool inside = x < tiles[index].width && y < tiles[index].height;
          image.pixels[(originY + y) * image.width + originX + x] =
            inside ? tiles[index].pixels[y * tiles[index].width + x] : 0xff000000;
        }
      }
    }
    return image;
  }

  auto memory = vulkan.replayRdram();
  auto hidden = vulkan.replayHiddenRdram();
  if(!memory) return image;
  //Crop to the scissor rectangle on both axes. Scissor coordinates are 10.2 fixed
  //point; the buffer's row stride stays colorWidth regardless of the crop.
  u32 stride = max(1u, state.colorWidth);
  auto region = rdpImageRegion(
    stride, state.scissorX0, state.scissorY0, state.scissorX1, state.scissorY1
  );
  u32 sourceX = region.x;
  u32 sourceY = region.y;
  image.width = region.width;
  image.height = region.height;
  image.pixels.resize(image.width * image.height, 0xff000000);

  //The replay RDRAM uses the same word-swapped layout as real RDRAM, so read it
  //through the same accessors rather than indexing the raw bytes.
  auto readByte = [&](u64 address) -> u8 {
    if(address >= memory->size) return 0;
    return memory->read<Byte>(address);
  };
  auto readHalf = [&](u64 address) -> u16 {
    if(address + 2 > memory->size) return 0;
    return memory->read<Half>(address);
  };
  auto readWord = [&](u64 address) -> u32 {
    if(address + 4 > memory->size) return 0;
    return memory->read<Word>(address);
  };
  u32 minimumDepth = 0x3ffff;
  u32 maximumDepth = 0;
  if(view == 1) {
    for(u32 y : range(image.height)) {
      for(u32 x : range(image.width)) {
        u64 sourceIndex = (u64)(y + sourceY) * stride + (x + sourceX);
        u16 encoded = readHalf(state.depthAddress + sourceIndex * 2) >> 2;
        if(encoded == 0x3fff) continue;
        u32 depth = rdpDecompressDepth(encoded);
        minimumDepth = min(minimumDepth, depth);
        maximumDepth = max(maximumDepth, depth);
      }
    }
  }

  for(u32 y : range(image.height)) {
    for(u32 x : range(image.width)) {
      u64 outputIndex = (u64)y * image.width + x;
      u64 sourceIndex = (u64)(y + sourceY) * stride + (x + sourceX);
      if(view == 0) {
        if(state.colorSize == 2 && state.colorFormat == 0) {
          u16 pixel = readHalf(state.colorAddress + sourceIndex * 2);
          image.pixels[outputIndex] = rdpDecodeRGBA16(pixel);
        } else if(state.colorSize == 3 && state.colorFormat == 0) {
          image.pixels[outputIndex] = 0xff000000
            | readWord(state.colorAddress + sourceIndex * 4) >> 8;
        } else if(state.colorSize == 1 && (state.colorFormat == 2 || state.colorFormat == 4)) {
          u64 address = state.colorAddress + sourceIndex;
          if(address < memory->size) {
            u8 intensity = readByte(address);
            image.pixels[outputIndex] =
              rdpDecodeGray(intensity);
          }
        }
      }
      if(view == 1) {
        u16 encoded = readHalf(state.depthAddress + sourceIndex * 2) >> 2;
        u8 intensity = 0;
        if(encoded != 0x3fff) {
          u32 depth = rdpDecompressDepth(encoded);
          u32 range = maximumDepth - minimumDepth;
          intensity = range ? 255 - (depth - minimumDepth) * 223 / range : 255;
        }
        image.pixels[outputIndex] =
          rdpDecodeGray(intensity);
      }
      if(view == 2 && hidden) {
        u64 colorAddress = state.colorAddress + sourceIndex * 2;
        u64 hiddenAddress = colorAddress >> 1;
        if(colorAddress + 2 <= memory->size
        && hiddenAddress < memory->size / 2) {
          u16 color = readHalf(colorAddress);
          u8 coverage = (color & 1) << 2 | (hidden[hiddenAddress] & 3);
          u8 intensity = coverage * 255 / 7;
          image.pixels[outputIndex] =
            rdpDecodeGray(intensity);
        }
      }
    }
  }
  return image;
  #endif
}

auto RDP::Debugger::resetCapture() -> void {
  capture = std::make_unique<Capture>();
  presentBuffer = ~0u;
  tickIdentifier = 0;
  tickCommands.clear();
  cachedCommands.clear();
  cachedCommands.resize(80);
  cachedCommandSequence = 0;
}

auto RDP::Debugger::beginCapture() -> void {
  capture->ready.store(false, std::memory_order_relaxed);
  capture->frame = {};
  capture->frame.identifier = nextCaptureIdentifier++;
  #if defined(VULKAN)
  vulkan.synchronize();
  if(vulkan.enable) {
    //Kept in parallel-RDP's native layout: it is handed straight back to set_tmem().
    capture->frame.tmem.resize(4_KiB);
    if(!vulkan.copyTMEM(capture->frame.tmem.data(), 4_KiB)) capture->frame.tmem.clear();
  }
  #endif
  capture->frame.rdram.assign(rdram.ram.data, rdram.ram.data + rdram.ram.size);
  capture->shadowRdram = capture->frame.rdram;
  if(rdram.hidden.data) {
    capture->frame.hiddenRdram.assign(
      rdram.hidden.data, rdram.hidden.data + rdram.ram.size / 2
    );
  }
  std::vector<const CachedCommand*> orderedState;
  for(auto& command : cachedCommands) {
    if(command.sequence) orderedState.push_back(&command);
  }
  std::sort(orderedState.begin(), orderedState.end(), [](auto lhs, auto rhs) {
    return lhs->sequence < rhs->sequence;
  });
  for(auto command : orderedState) {
    capture->frame.initialCommands.push_back(command->words);
  }
  capture->active = true;
  capture->inCommandList = false;
  capture->activeFields = 0;
  for(u32 address : range(14)) {
    captureViRegister(address, vi.readWord(address * 4, vi));
  }
}

auto RDP::Debugger::endCapture() -> void {
  commandBatch();
  capture->active = false;
  capture->shadowRdram.clear();
  capture->ready.store(true, std::memory_order_release);
}

auto RDP::Debugger::presentBoundary(u32 origin) -> void {
  u32 buffer = origin >> 12;
  if(buffer == presentBuffer) return;
  presentBuffer = buffer;
  if(!capture) return;

  if(capture->active) {
    if(capture->frame.commandOffsets.empty()) return;
    endCapture();
    return;
  }

  if(!capture->armed.exchange(false, std::memory_order_acquire)) return;
  beginCapture();
}

auto RDP::Debugger::frameBoundary() -> void {
  if(!capture) return;
  static constexpr u32 fallbackFields = 4;

  if(capture->active) {
    if(capture->frame.commandOffsets.empty()) {
      beginCapture();
      return;
    }
    if(++capture->activeFields >= fallbackFields) endCapture();
    return;
  }

  if(!capture->armed.load(std::memory_order_acquire)) return;
  if(++capture->armedFields < fallbackFields) return;
  if(!capture->armed.exchange(false, std::memory_order_acquire)) return;
  beginCapture();
}

auto RDP::Debugger::commandBatch() -> void {
  if(!capture || !capture->active) return;
  if(capture->inCommandList) return;
  capture->inCommandList = true;

  #if defined(VULKAN)
  if(vulkan.enable) vulkan.synchronize();
  #endif

  static constexpr u32 blockSize = 4_KiB;
  for(u32 address = 0; address < rdram.ram.size; address += blockSize) {
    u32 size = min(blockSize, rdram.ram.size - address);
    if(std::memcmp(
      rdram.ram.data + address, capture->shadowRdram.data() + address, size
    ) == 0) continue;

    RDPFrameCapture::Packet packet;
    packet.type = RDPFrameCapture::Packet::Type::DramDiff;
    packet.address = address;
    packet.bytes.assign(rdram.ram.data + address, rdram.ram.data + address + size);
    capture->frame.packets.push_back(std::move(packet));
    std::memcpy(
      capture->shadowRdram.data() + address, rdram.ram.data + address, size
    );
  }
}

auto RDP::Debugger::captureCommand(const u32* words, u32 wordCount) -> void {
  if(!capture || !capture->active) return;

  capture->frame.commandOffsets.push_back(capture->frame.packets.size());
  RDPFrameCapture::Packet packet;
  packet.type = RDPFrameCapture::Packet::Type::Commands;
  packet.words.assign(words, words + wordCount);
  capture->frame.packets.push_back(std::move(packet));

  if((words[0] >> 24 & 63) == 0x29) capture->inCommandList = false;
}

auto RDP::Debugger::captureViRegister(u32 address, u32 value) -> void {
  if(!capture || !capture->active) return;

  RDPFrameCapture::Packet packet;
  packet.type = RDPFrameCapture::Packet::Type::ViRegister;
  packet.address = address;
  packet.value = value;
  capture->frame.packets.push_back(std::move(packet));
}

auto RDP::Debugger::captureScanout(bool field) -> void {
  if(!capture || !capture->active) return;

  commandBatch();
  RDPFrameCapture::Packet packet;
  packet.type = RDPFrameCapture::Packet::Type::Scanout;
  packet.value = field;
  capture->frame.packets.push_back(std::move(packet));
}
