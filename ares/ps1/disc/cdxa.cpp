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

auto Disc::CDXA::clockSector() -> void {
  n8 subMode     = drive->sector.data[18];
  n1 endOfRecord = subMode.bit(0);
  n1 video       = subMode.bit(1);
  n1 audio       = subMode.bit(2);
  n1 data        = subMode.bit(3);
  n1 trigger     = subMode.bit(4);
  n1 form2       = subMode.bit(5);
  n1 realTime    = subMode.bit(6);
  n1 endOfFile   = subMode.bit(7);

  n8  codingInfo    = drive->sector.data[19];
  n1  stereo        = codingInfo.bit(0);
  n16 sampleRate    = codingInfo.bit(2) ? 18900 : 37800;
  n32 bitsPerSample = codingInfo.bit(4) ? 8 : 4;
  n1  emphasis      = codingInfo.bit(6);

  if(stereo == 0 && bitsPerSample == 4) decodeADPCM<0, 0>();
  if(stereo == 0 && bitsPerSample == 8) decodeADPCM<0, 1>();
  if(stereo == 1 && bitsPerSample == 4) decodeADPCM<1, 0>();
  if(stereo == 1 && bitsPerSample == 8) decodeADPCM<1, 1>();

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
    sample.left  = 0;
    sample.right = 0;
  } else {
    //each channel is saturated; but the combination of each channel is not and may overflow
    sample.left  = sclamp<16>(left * self.audio.volume[0] >> 7) + sclamp<16>(right * self.audio.volume[2] >> 7);
    sample.right = sclamp<16>(left * self.audio.volume[1] >> 7) + sclamp<16>(right * self.audio.volume[3] >> 7);
  }

//stream->sample(sample.left / 32768.0, sample.right / 32768.0);
}

template<bool isStereo, bool is8bit>
auto Disc::CDXA::decodeADPCM() -> void {
  const u32 Blocks = 18;
  const u32 BlockSize = 128;
  const u32 WordsPerBlock = 28;
  const u32 SamplesPerBlock = WordsPerBlock * (is8bit ? 4 : 8);

  s16 output[SamplesPerBlock];
  for(u32 block : range(Blocks)) {
    decodeBlock<isStereo, is8bit>(output, 24 + block * BlockSize);
    for(auto sample : output) {
      if(!samples.full()) samples.write(sample);
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
      s16 sample = s16(nibble << 12) >> shift;

      s32* previous = isStereo ? &previousSamples[(block & 1) * 2] : &previousSamples[0];
      s32 interpolated = s32(sample) + ((previous[0] * positive) + (previous[1] * negative) + 32) / 64;
      previous[1] = previous[0];
      previous[0] = interpolated;

      output[index] = sclamp<16>(interpolated);
      index += isStereo ? 2 : 1;
    }
  }
}
