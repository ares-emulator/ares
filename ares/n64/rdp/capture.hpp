#pragma once

//Length of each RDP opcode in 64-bit command words. Shared by both command
//consumers so capture boundaries cannot differ between renderers.
inline constexpr u32 rdpCommandLength(u32 opcode) {
  constexpr u8 lengths[64] = {
    1, 1, 1, 1, 1, 1, 1, 1, 4, 6,12,14,12,14,20,22,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };
  return lengths[opcode & 63];
}

inline constexpr u32 rdpDecodeRGBA16(u16 pixel) {
  u8 r = pixel >> 11 & 31; r = r << 3 | r >> 2;
  u8 g = pixel >>  6 & 31; g = g << 3 | g >> 2;
  u8 b = pixel >>  1 & 31; b = b << 3 | b >> 2;
  return 0xff000000 | r << 16 | g << 8 | b;
}

inline constexpr u32 rdpDecodeGray(u8 intensity) {
  return 0xff000000 | intensity << 16 | intensity << 8 | intensity;
}

inline constexpr u32 rdpDecompressDepth(u16 encoded) {
  u32 exponent = encoded >> 11;
  u32 mantissa = encoded & 0x7ff;
  u32 shift = exponent < 6 ? 6 - exponent : 0;
  return (mantissa << shift) + 0x40000 - (0x40000 >> exponent);
}

struct RDPImageRegion {
  u32 x = 0;
  u32 y = 0;
  u32 width = 1;
  u32 height = 1;
};

inline auto rdpImageRegion(
  u32 stride, u32 x0, u32 y0, u32 x1, u32 y1
) -> RDPImageRegion {
  RDPImageRegion region;
  stride = stride ? stride : 1;
  region.x = min(stride - 1, x0 >> 2);
  region.y = y0 >> 2;
  region.width = x1 > x0 ? (x1 - x0 + 3) >> 2 : stride;
  region.width = max(1u, min(region.width, stride - region.x));
  region.height = y1 > y0 ? (y1 - y0 + 3) >> 2 : 1;
  region.height = min(480u, max(1u, region.height));
  return region;
}

struct RDPFrameCapture {
  u64 identifier = 0;

  struct Packet {
    enum class Type : u32 { Commands, DramDiff, ViRegister, Scanout };

    Type type = Type::Commands;
    u32 address = 0;
    u32 value = 0;
    std::vector<u32> words;
    std::vector<u8> bytes;
  };

  std::vector<u8> rdram;
  std::vector<u8> hiddenRdram;
  std::vector<u8> tmem;
  std::vector<std::vector<u32>> initialCommands;
  std::vector<Packet> packets;
  std::vector<u32> commandOffsets;
};

//RDP register file as of a given command, latched by replaying the state-setting
//commands of a capture. Decoded from the command words rather than read back
//from parallel-RDP, so it reports what the game wrote.
struct RDPCaptureState {
  struct Tile {
    u32  format = 0, size = 0, line = 0, address = 0, palette = 0;
    u32  maskS = 0, shiftS = 0, maskT = 0, shiftT = 0;
    bool clampS = false, mirrorS = false, clampT = false, mirrorT = false;
    u32  s0 = 0, t0 = 0, s1 = 0, t1 = 0;  //10.2 fixed point
    bool set = false;
  };

  u32 colorAddress = 0, colorFormat = 0, colorSize = 0, colorWidth = 1;
  u32 depthAddress = 0;
  u32 textureAddress = 0, textureFormat = 0, textureSize = 0, textureWidth = 1;
  u32 scissorX0 = 0, scissorY0 = 0, scissorX1 = 0, scissorY1 = 4;  //10.2
  bool scissorField = false, scissorOdd = false;
  u64 otherModes = 0;
  u64 combine = 0;
  u32 fillColor = 0, fogColor = 0, blendColor = 0, primColor = 0, envColor = 0;
  u32 primMinLevel = 0, primLevelFraction = 0;
  u32 primDepthZ = 0, primDepthDZ = 0;
  u32 keyR = 0, keyGB = 0;
  u64 convert = 0;
  Tile tiles[8];
};
