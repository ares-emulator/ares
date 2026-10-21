#pragma once
// Semantic decoder from ares-64 1c9b9637 ares/n64/rdp/decode.cpp.
namespace CaptureRdpDecode {
using namespace nall;
static const char* formatNames[] = {"RGBA","YUV","CI","IA","I","I","I","I"};
static const char* sizeNames[]  = {"4bpp","8bpp","16bpp","32bpp"};
static const char* cycleNames[] = {"1cycle","2cycle","Copy","Fill"};
static const char* zModeNames[] = {"Opaque","Interpen.","Transp.","Decal"};

// Color Combiner parameter names (from wiki)
static const char* ccRGB_A[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","1","NOISE", "0"};
static const char* ccRGB_B[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","CENTER","K4", "0"};
static const char* ccRGB_C[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","SCALE","COMBINED_ALPHA","TEX0_ALPHA","TEX1_ALPHA","PRIM_ALPHA","SHADE_ALPHA","ENV_ALPHA","LOD_FRAC","PRIM_LOD_FRAC","K5", "0"};
static const char* ccRGB_D[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","1","0", "0"};
static const char* ccAlpha_A[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","1","0"};
static const char* ccAlpha_B[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","1","0"};
static const char* ccAlpha_C[] = {"LOD_FRAC","TEX0","TEX1","PRIM","SHADE","ENV","PRIM_LOD_FRAC","0"};
static const char* ccAlpha_D[] = {"COMBINED","TEX0","TEX1","PRIM","SHADE","ENV","1","0"};

static auto ccName(const char** table, u32 val, u32 max) -> const char* {
  if(val >= max) return table[max - 1];
  return table[val];
}

auto rdpCommandDescription(u8 opcode, u64 w0) -> string {
  #define u(x, hi, lo) ((u##x)(w0 >> (lo) & ((1ull << ((hi)-(lo)+1)) - 1)))

  switch(opcode) {
  case 0x00: return "No Operation";
  case 0x26: return "Sync Load (25 GCLK stall)";
  case 0x27: return "Sync Pipe (50 GCLK stall)";
  case 0x28: return "Sync Tile (33 GCLK stall)";
  case 0x29: return "Sync Full (flush + DP interrupt)";

  case 0x2a: // Set_Key_GB
    return {"G: width=", u(12,55,44), " center=", u(8,23,16), " scale=", u(8,15,8),
            "  B: width=", u(12,43,32), " center=", u(8,7,0), " scale=", u(8,31,24)};

  case 0x2b: // Set_Key_R
    return {"R: width=", u(12,27,16), " center=", u(8,7,0), " scale=", u(8,15,8)};

  case 0x2c: // Set_Convert
    return {"K0=", u(9,53,45), " K1=", u(9,44,36), " K2=", u(9,35,27),
            " K3=", u(9,26,18), " K4=", u(9,17,9), " K5=", u(9,8,0)};

  case 0x2d: // Set_Scissor
    return {"ul=(", u(12,55,44), ".0, ", u(12,43,32), ".0)  lr=(", u(12,23,12), ".0, ", u(12,11,0),
            ".0)  field=", u(1,25,25) ? "even/odd":"progressive", " odd=", u(1,24,24)};

  case 0x2e: // Set_Primitive_Depth
    return {"z=", u(16,31,16), "  dz=", u(16,15,0)};

  case 0x2f: { // Set_Other_Modes
    u8 cy = u(2,53,52);
    u8 zm = u(2,11,10);
    string s = {cycleNames[cy]};
    if(u(1,51,51)) s.append(" persp");
    if(u(1,50,50)) s.append(" detail");
    if(u(1,49,49)) s.append(" sharpen");
    if(u(1,48,48)) s.append(" LOD");
    if(u(1,47,47)) s.append(" TLUT");
    s.append(u(1,45,45) ? " bilinear" : " point");
    if(u(1,44,44)) s.append(" midTex");
    if(u(1,43,43)) s.append(" bilerp0");
    if(u(1,42,42)) s.append(" bilerp1");
    if(u(1,41,41)) s.append(" conv1");
    if(u(1,40,40)) s.append(" key");
    s.append(" rDith=", u(2,39,38), " aDith=", u(2,37,36));
    s.append("\nz=", zModeNames[zm]);
    if(u(1,5,5))  s.append(" zUpd");
    if(u(1,4,4))  s.append(" zCmp");
    if(u(1,3,3))  s.append(" aa");
    if(u(1,2,2))  s.append(" zSrc=Prim"); else s.append(" zSrc=Pix");
    if(u(1,14,14)) s.append(" forceBlend");
    if(u(1,13,13)) s.append(" aCvg");
    if(u(1,12,12)) s.append(" cvgXa");
    if(u(1,1,1))  s.append(" dithA");
    if(u(1,0,0))  s.append(" aCmp");
    return s;
  }

  case 0x30: // Load_TLUT
    return {"tile=", u(3,26,24), "  ul=(", u(12,55,44), ",", u(12,43,32),
            ")  lr=(", u(12,23,12), ",", u(12,11,0), ")"};

  case 0x32: // Set_Tile_Size
    return {"idx=", u(3,26,24), "  ul=(", u(12,55,44), ",", u(12,43,32),
            ")  lr=(", u(12,23,12), ",", u(12,11,0), ")"};

  case 0x33: // Load_Block
    return {"tile=", u(3,26,24), "  ul=(", u(12,55,44), ",", u(12,43,32),
            ")  lr.s=", u(12,23,12), "  dxt=", u(12,11,0)};

  case 0x34: // Load_Tile
    return {"tile=", u(3,26,24), "  ul=(", u(12,55,44), ",", u(12,43,32),
            ")  lr=(", u(12,23,12), ",", u(12,11,0), ")"};

  case 0x35: { // Set_Tile
    u8 fmt = u(3,55,53); u8 sz = u(2,52,51);
    return {"idx=", u(3,26,24), "  ", formatNames[fmt], " ", sizeNames[sz],
            "  line=", u(9,50,41), "  addr=0x", hex(u(9,40,32), 3L),
            "  pal=", u(4,23,20),
            "  S:[c=", u(1,9,9) ? "clamp":"wrap", " m=", u(1,8,8) ? "mirror":"-",
            " mk=", u(4,7,4), " sh=", u(4,3,0), "]",
            "  T:[c=", u(1,19,19) ? "clamp":"wrap", " m=", u(1,18,18) ? "mirror":"-",
            " mk=", u(4,17,14), " sh=", u(4,13,10), "]"};
  }

  case 0x36: // Fill_Rectangle
    return {"ul=(", u(12,23,12), ".0,", u(12,11,0), ".0)  lr=(", u(12,55,44), ".0,", u(12,43,32), ".0)"};

  case 0x37: // Set_Fill_Color
    return {hex(u(8,31,24), 2L), " ", hex(u(8,23,16), 2L), " ", hex(u(8,15,8), 2L),
            " | ", hex(u(8,7,0), 2L)};

  case 0x38: // Set_Fog_Color
    return {hex(u(8,31,24), 2L), " ", hex(u(8,23,16), 2L), " ", hex(u(8,15,8), 2L),
            " | ", hex(u(8,7,0), 2L)};

  case 0x39: // Set_Blend_Color
    return {hex(u(8,31,24), 2L), " ", hex(u(8,23,16), 2L), " ", hex(u(8,15,8), 2L),
            " | ", hex(u(8,7,0), 2L)};

  case 0x3a: // Set_Primitive_Color
    return {hex(u(8,31,24), 2L), " ", hex(u(8,23,16), 2L), " ", hex(u(8,15,8), 2L),
            " | ", hex(u(8,7,0), 2L), "  minLOD=", u(8,47,40), " lodFrac=", u(8,39,32)};

  case 0x3b: // Set_Environment_Color
    return {hex(u(8,31,24), 2L), " ", hex(u(8,23,16), 2L), " ", hex(u(8,15,8), 2L),
            " | ", hex(u(8,7,0), 2L)};

  case 0x3c: { // Set_Combine_Mode
    //Bit layout per the RDP Set_Combine_Mode command (see parallel-rdp op_set_combine).
    u32 a0  = u(4,55,52), c0  = u(5,51,47), Aa0 = u(3,46,44), Ac0 = u(3,43,41);
    u32 b0  = u(4,31,28), d0  = u(3,17,15), Ab0 = u(3,14,12), Ad0 = u(3,11,9);
    u32 a1  = u(4,40,37), c1  = u(5,36,32), Aa1 = u(3,23,21), Ac1 = u(3,20,18);
    u32 b1  = u(4,27,24), d1  = u(3,8,6),   Ab1 = u(3,5,3),   Ad1 = u(3,2,0);

    return {"RGB: (", ccName(ccRGB_A, a0, 9), "-", ccName(ccRGB_B, b0, 9),
            ")*", ccName(ccRGB_C, c0, 17), "+", ccName(ccRGB_D, d0, 8),
            "    A: (", ccName(ccAlpha_A, Aa0, 8), "-", ccName(ccAlpha_B, Ab0, 8),
            ")*", ccName(ccAlpha_C, Ac0, 8), "+", ccName(ccAlpha_D, Ad0, 8),
            "\nRGB: (", ccName(ccRGB_A, a1, 9), "-", ccName(ccRGB_B, b1, 9),
            ")*", ccName(ccRGB_C, c1, 17), "+", ccName(ccRGB_D, d1, 8),
            "    A: (", ccName(ccAlpha_A, Aa1, 8), "-", ccName(ccAlpha_B, Ab1, 8),
            ")*", ccName(ccAlpha_C, Ac1, 8), "+", ccName(ccAlpha_D, Ad1, 8)};
  }

  case 0x3d: // Set_Texture_Image
    return {formatNames[u(3,55,53)], " ", sizeNames[u(2,52,51)],
            "  w=", u(10,42,32)+1, "  addr=0x", hex(u(26,25,0), 8L)};

  case 0x3e: // Set_Mask_Image (Depth)
    return {"addr=0x", hex(u(26,25,0), 8L)};

  case 0x3f: // Set_Color_Image
    return {formatNames[u(3,55,53)], " ", sizeNames[u(2,52,51)],
            "  w=", u(10,42,32)+1, "  addr=0x", hex(u(26,25,0), 8L)};

  case 0x08: case 0x09: case 0x0a: case 0x0b:
  case 0x0c: case 0x0d: case 0x0e: case 0x0f:
    return {u(1,55,55) ? "Left":"Right",
            "  LOD=", u(3,54,51), "  tile=", u(3,50,48),
            "  y:[", u(14,47,32), ",", u(14,31,16), ",", u(14,15,0), "]"};

  case 0x24:
    return {"TexRect  tile=", u(3,26,24),
            "  ul=(", u(12,23,12), ".0,", u(12,11,0), ".0)",
            "  lr=(", u(12,55,44), ".0,", u(12,43,32), ".0)"};
  case 0x25:
    return {"TexRectFlip  tile=", u(3,26,24),
            "  ul=(", u(12,23,12), ".0,", u(12,11,0), ".0)",
            "  lr=(", u(12,55,44), ".0,", u(12,43,32), ".0)"};

  default:
    return "Invalid";
  }

  #undef u
}

}
