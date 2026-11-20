auto Disc::CDXA::load(Node::Object parent) -> void {
//stream = parent->append<Node::Audio::Stream>("CD-XA");
//stream->setChannels(2);
//stream->setFrequency(37800);
//stream->setResamplerFrequency(44100);
}

auto Disc::CDXA::unload(Node::Object parent) -> void {
//parent->remove(stream);
//stream.reset();
}

auto Disc::CDXA::reset() -> void {
  current = {};
  sample = {};
  monaural = false;
  samples.flush();
  for(auto& previous : previousSamples) previous = 0;
  resampler = {};
  resampler.phase = 6;
}

auto Disc::CDXA::clockSector() -> void {
  u8 file = drive->sector.data[16];
  u8 channel = drive->sector.data[17];
  if(!current.active) {
    if(channel == 0xff && (!drive->mode.xaFilter || filter.channel != 0xff)) return;
    current.file = file;
    current.channel = channel;
    current.active = 1;
  } else if(current.file != file || current.channel != channel) {
    return;
  }
  n8 subMode     = drive->sector.data[18];
  n1 endOfRecord = subMode.bit(0);
  n1 video       = subMode.bit(1);
  n1 audio       = subMode.bit(2);
  n1 data        = subMode.bit(3);
  n1 trigger     = subMode.bit(4);
  n1 form2       = subMode.bit(5);
  n1 realTime    = subMode.bit(6);
  n1 endOfFile   = subMode.bit(7);

  if(endOfFile) current = {};

  if(!samples.empty()) return;

  n8  codingInfo    = drive->sector.data[19];
  n1  stereo        = codingInfo.bit(0);
  n1  halfSampleRate= codingInfo.bit(2);
  n32 bitsPerSample = codingInfo.bit(4) ? 8 : 4;
  n1  emphasis      = codingInfo.bit(6);

  if(stereo == 0 && bitsPerSample == 4) decodeADPCM<0, 0>(halfSampleRate);
  if(stereo == 0 && bitsPerSample == 8) decodeADPCM<0, 1>(halfSampleRate);
  if(stereo == 1 && bitsPerSample == 4) decodeADPCM<1, 0>(halfSampleRate);
  if(stereo == 1 && bitsPerSample == 8) decodeADPCM<1, 1>(halfSampleRate);

  monaural = !stereo;
}

auto Disc::CDXA::clockSample() -> void {
  s16 left  = 0;
  s16 right = 0;

  if(monaural) {
    left = right = samples.read(0);
  } else {
    left  = samples.read(0);
    right = samples.read(0);
  }

  if(self.audio.mute || self.audio.muteADPCM) {
    resample(0, 0);
  } else {
    resample(
      sclamp<16>((left * self.audio.volume[0] >> 7) + (right * self.audio.volume[2] >> 7)),
      sclamp<16>((left * self.audio.volume[1] >> 7) + (right * self.audio.volume[3] >> 7))
    );
  }
}

auto Disc::CDXA::resample(s16 left, s16 right) -> void {
  static constexpr s16 zigzag[29][7] = {
    {  0x0000,  0x0000,  0x0000,  0x0000, -0x0001,  0x0002, -0x0005},
    {  0x0000,  0x0000,  0x0000, -0x0001,  0x0003, -0x0008,  0x0011},
    {  0x0000,  0x0000, -0x0001,  0x0003, -0x0008,  0x0010, -0x0023},
    {  0x0000, -0x0002,  0x0003, -0x0008,  0x0011, -0x0023,  0x0046},
    {  0x0000,  0x0000, -0x0002,  0x0006, -0x0010,  0x002b, -0x0017},
    { -0x0002,  0x0003, -0x0005,  0x0005,  0x000a,  0x001a, -0x0044},
    {  0x000a, -0x0013,  0x001f, -0x001b,  0x006b, -0x00eb,  0x015b},
    { -0x0022,  0x003c, -0x004a,  0x00a6, -0x016d,  0x027b, -0x0347},
    {  0x0041, -0x004b,  0x00b3, -0x01a8,  0x0350, -0x0548,  0x080e},
    { -0x0054,  0x00a2, -0x0192,  0x0372, -0x0623,  0x0afa, -0x1249},
    {  0x0034, -0x00e3,  0x02b1, -0x05bf,  0x0bcd, -0x16fa,  0x3c07},
    {  0x0009,  0x0132, -0x039e,  0x09b8, -0x1780,  0x53e0,  0x53e0},
    { -0x010a, -0x0043,  0x04f8, -0x11b4,  0x6794,  0x3c07, -0x16fa},
    {  0x0400, -0x0267, -0x05a6,  0x74bb,  0x234c, -0x1249,  0x0afa},
    { -0x0a78,  0x0c9d,  0x7939,  0x0c9d, -0x0a78,  0x080e, -0x0548},
    {  0x234c,  0x74bb, -0x05a6, -0x0267,  0x0400, -0x0347,  0x027b},
    {  0x6794, -0x11b4,  0x04f8, -0x0043, -0x010a,  0x015b, -0x00eb},
    { -0x1780,  0x09b8, -0x039e,  0x0132,  0x0009, -0x0044,  0x001a},
    {  0x0bcd, -0x05bf,  0x02b1, -0x00e3,  0x0034, -0x0017,  0x002b},
    { -0x0623,  0x0372, -0x0192,  0x00a2, -0x0054,  0x0046, -0x0023},
    {  0x0350, -0x01a8,  0x00b3, -0x004b,  0x0041, -0x0023,  0x0010},
    { -0x016d,  0x00a6, -0x004a,  0x003c, -0x0022,  0x0011, -0x0008},
    {  0x006b, -0x001b,  0x001f, -0x0013,  0x000a, -0x0005,  0x0002},
    {  0x000a,  0x0005, -0x0005,  0x0003, -0x0001,  0x0000,  0x0000},
    { -0x0010,  0x0006, -0x0002,  0x0000,  0x0000,  0x0000,  0x0000},
    {  0x0011, -0x0008,  0x0003, -0x0002,  0x0001,  0x0000,  0x0000},
    { -0x0008,  0x0003, -0x0001,  0x0000,  0x0000,  0x0000,  0x0000},
    {  0x0003, -0x0001,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000},
    { -0x0001,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000},
  };

  resampler.ring[0][resampler.position & 31] = left;
  resampler.ring[1][resampler.position & 31] = right;
  resampler.position++;
  if(--resampler.phase) return;
  resampler.phase = 6;

  for(u32 table : range(7)) {
    u32 slot = resampler.head + resampler.count & 31;
    if(resampler.count == 32) break;
    for(u32 channel : range(2)) {
      s32 sum = 0;
      for(u32 index : range(29)) {
        sum += resampler.ring[channel][resampler.position - 1 - index & 31] * zigzag[index][table] >> 15;
      }
      resampler.frames[slot][channel] = sclamp<16>(sum);
    }
    resampler.count++;
  }
}

auto Disc::CDXA::pullSample() -> void {
  if(!resampler.count) {
    sample.left = 0;
    sample.right = 0;
    return;
  }
  sample.left  = resampler.frames[resampler.head][0];
  sample.right = resampler.frames[resampler.head][1];
  resampler.head = resampler.head + 1 & 31;
  resampler.count--;
}

template<bool isStereo, bool is8bit>
auto Disc::CDXA::decodeADPCM(n1 halfSampleRate) -> void {
  const u32 Blocks = 18;
  const u32 BlockSize = 128;
  const u32 WordsPerBlock = 28;
  const u32 SamplesPerBlock = WordsPerBlock * (is8bit ? 4 : 8);

  if(samples.size() + Blocks * SamplesPerBlock * (halfSampleRate ? 2 : 1) > samples.capacity()) return;

  s16 output[SamplesPerBlock];
  for(u32 block : range(Blocks)) {
    decodeBlock<isStereo, is8bit>(output, 24 + block * BlockSize);
    for(auto sample : output) {
      if(!samples.full()) samples.write(sample);
      if(halfSampleRate && !samples.full()) samples.write(sample);
    }
  }
}

template<bool isStereo, bool is8bit>
auto Disc::CDXA::decodeBlock(s16* output, u16 address) -> void {
  static constexpr s32 filterPositive[] = {0, 60, 115, 98};
  static constexpr s32 filterNegative[] = {0, 0, -52, -55};
  static constexpr u32 Blocks = is8bit ? 4 : 8;
  static constexpr u32 WordsPerBlock = 28;

  for(u32 block : range(Blocks)) {
    u8  header   = drive->sector.data[address + 4 + block];
    u8  shift    = (header & 0x0f) > 12 ? 9 : (header & 0x0f);
    u8  filter   = (header & 0x30) >> 4;
    s32 positive = filterPositive[filter];
    s32 negative = filterNegative[filter];
    u16 index    = isStereo ? (block >> 1) * (WordsPerBlock << 1) + (block & 1) : block * WordsPerBlock;

    for(u32 word : range(WordsPerBlock)) {
      u32 data = 0;
      data |= drive->sector.data[address + 16 + word * 4 + 0] <<  0;
      data |= drive->sector.data[address + 16 + word * 4 + 1] <<  8;
      data |= drive->sector.data[address + 16 + word * 4 + 2] << 16;
      data |= drive->sector.data[address + 16 + word * 4 + 3] << 24;

      u32 nibble = is8bit ? (data >> block * 8 & 0xff) : (data >> block * 4 & 0x0f);
      s16 sample = is8bit ? s16(nibble << 8) >> shift : s16(nibble << 12) >> shift;

      s32* previous = isStereo ? &previousSamples[(block & 1) * 2] : &previousSamples[0];
      s32 interpolated = sclamp<16>(s32(sample) + ((previous[0] * positive) + (previous[1] * negative) + 32) / 64);
      previous[1] = previous[0];
      previous[0] = interpolated;

      output[index] = interpolated;
      index += isStereo ? 2 : 1;
    }
  }
}
