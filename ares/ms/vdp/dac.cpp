auto VDP::DAC::setup(n8 y) -> void {
  output = vdp.screen->pixels().data() + (24 + y) * 256;
}

auto VDP::DAC::run(n8 x, n8 y) -> void {
  n12 color = palette(16 | io.backdropColor);
  if(!io.leftClip || x >= 8) {
    if(vdp.background.output.priority || !vdp.sprite.output.color) {
      color = palette(vdp.background.output.palette << 4 | vdp.background.output.color);
    } else if(vdp.sprite.output.color) {
      color = palette(16 | vdp.sprite.output.color);
    }
  }
  if(!io.displayEnable) color = 0;
  output[x] = color;
}

auto VDP::DAC::palette(n5 index) -> n12 {
  //TMS9918A colors are approximated by converting to RGB6 palette colors
  static const n6 palette[16] = {
    0x00, 0x00, 0x08, 0x0c, 0x10, 0x30, 0x01, 0x3c,
    0x02, 0x03, 0x05, 0x0f, 0x04, 0x33, 0x15, 0x3f,
  };
  if(Model::MarkIII() || Model::MasterSystem()) {
    if(!vdp.mode().bit(3)) return palette[index.bit(0,3)];
    return vdp.cram[index].bit(0,5);
  }
  if(Model::GameGearMS()) {
    n6 color = vdp.cram[index];
    if(!vdp.mode().bit(3)) color = palette[index.bit(0,3)];
    n4 r = color.bit(0,1) << 0 | color.bit(0,1) << 2;
    n4 g = color.bit(2,3) << 0 | color.bit(2,3) << 2;
    n4 b = color.bit(4,5) << 0 | color.bit(4,5) << 2;
    return r << 0 | g << 4 | b << 8;
  }
  if(Model::GameGear()) {
    if(!vdp.mode().bit(3)) {
      n6 color = palette[index.bit(0,3)];
      n4 r = color.bit(0,1) << 0 | color.bit(0,1) << 2;
      n4 g = color.bit(2,3) << 0 | color.bit(2,3) << 2;
      n4 b = color.bit(4,5) << 0 | color.bit(4,5) << 2;
      return r << 0 | g << 4 | b << 8;
    }
    return vdp.cram[index * 2 + 0] << 0 | vdp.cram[index * 2 + 1] << 8;
  }
  return 0;
}

auto VDP::DAC::power() -> void {
  io = {};
}
