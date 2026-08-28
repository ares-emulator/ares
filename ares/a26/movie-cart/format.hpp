namespace MVC {

static constexpr u32 FieldSize = 4_KiB;
static constexpr u32 MinimumSize = 2 * FieldSize;

struct FieldDescriptor {
  bool valid = false;
  bool extended = false;
  u8 signatureOffset = 0;
  u8 vsync = 0;
  u8 vblank = 0;
  u8 overscan = 0;
  u8 visible = 0;
  u8 rate = 0;
  u8 embeddedFrame = 0;
  u16 audioOffset = 0;
  u16 audioCount = 0;
  u16 graphOffset = 0;
  u16 graphCount = 0;
  u16 colorOffset = 0;
  u16 colorCount = 0;
  u16 backgroundOffset = 0;
  u16 backgroundCount = 0;
  u16 timecodeOffset = 0;
  u16 timecodeCount = 0;

  auto operator==(const FieldDescriptor&) const -> bool = default;
};

inline auto signatureOffset(std::span<const u8> field) -> i32 {
  for(u32 offset : range(2)) {
    if(field.size() < offset + 4) continue;
    if(field[offset + 0] == 'M' && field[offset + 1] == 'V'
      && field[offset + 2] == 'C' && field[offset + 3] == 0) return offset;
  }
  return -1;
}

inline auto fingerprintPrefix(Hash::SHA256& hash, u64 size, std::span<const u8> field0,
  std::span<const u8> field1) -> bool {
  if(field0.size() != FieldSize || field1.size() != FieldSize) return false;
  for(auto byte : std::span<const u8>{(const u8*)"ares-mvc-v1", 11}) hash.input(byte);
  for(u32 byte : range(8)) hash.input(size >> (byte * 8));
  for(auto byte : field0) hash.input(byte);
  for(auto byte : field1) hash.input(byte);
  return true;
}

inline auto parseField(std::span<const u8> field, FieldDescriptor& descriptor) -> bool {
  descriptor = {};
  if(field.size() != FieldSize) return false;
  auto base = signatureOffset(field);
  if(base < 0) return false;

  descriptor.signatureOffset = base;
  auto formatOffset = (u32)base + 4;
  if(formatOffset >= field.size()) return false;
  descriptor.extended = field[formatOffset] & 0x80;

  u32 cursor = 0;
  if(descriptor.extended) {
    if((field[formatOffset] & 0x7f) != 0 || (u32)base + 15 > field.size()) return false;
    descriptor.vsync = field[base + 9];
    descriptor.vblank = field[base + 10];
    descriptor.overscan = field[base + 11];
    descriptor.visible = field[base + 12];
    descriptor.rate = field[base + 13];
    descriptor.embeddedFrame = field[base + 8] + 1;
    cursor = base + 14;
  } else {
    if((u32)base + 7 > field.size()) return false;
    descriptor.vsync = 3;
    descriptor.vblank = 37;
    descriptor.overscan = 30;
    descriptor.visible = 192;
    descriptor.rate = 60;
    descriptor.embeddedFrame = field[base + 6];
    cursor = base + 7;
  }

  auto totalLines = (u32)descriptor.vsync + descriptor.vblank + descriptor.overscan + descriptor.visible;
  if(!descriptor.vsync || !descriptor.visible || !descriptor.rate || totalLines > 512) return false;
  auto graphCount = 5u * descriptor.visible;
  auto colorCount = 5u * descriptor.visible;
  auto backgroundCount = (u32)descriptor.visible;
  auto timecodeCount = 60u;

  auto take = [&](u32 count, u16& offset, u16& size) -> bool {
    if(cursor > field.size() || count > field.size() - cursor) return false;
    offset = cursor;
    size = count;
    cursor += count;
    return true;
  };

  if(!take(totalLines, descriptor.audioOffset, descriptor.audioCount)) return false;
  if(!take(graphCount, descriptor.graphOffset, descriptor.graphCount)) return false;
  if(descriptor.extended) {
    if(!take(colorCount, descriptor.colorOffset, descriptor.colorCount)) return false;
    if(!take(backgroundCount, descriptor.backgroundOffset, descriptor.backgroundCount)) return false;
    if(!take(timecodeCount, descriptor.timecodeOffset, descriptor.timecodeCount)) return false;
  } else {
    if(!take(timecodeCount, descriptor.timecodeOffset, descriptor.timecodeCount)) return false;
    if(!take(colorCount, descriptor.colorOffset, descriptor.colorCount)) return false;
    if(!take(backgroundCount, descriptor.backgroundOffset, descriptor.backgroundCount)) return false;
  }

  descriptor.valid = true;
  return true;
}

}
