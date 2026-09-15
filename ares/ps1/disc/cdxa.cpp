#include "xa-resampler.hpp"

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
  u8 file = drive->sector.data[16];
  u8 channel = drive->sector.data[17];
  if(drive->mode.xaFilter && (file != filter.file || channel != filter.channel)) return;
  if(!current.selected) {
    // R2 reference compatibility: ignore unselected channel255 unless explicitly requested.
    if(channel == 255 && (!drive->mode.xaFilter || filter.channel != 255)) return;
    current = {file, channel, true};
  } else if(file != current.file || channel != current.channel) {
    return;
  }
  if(drive->sector.data[18] & 0x80) current = {};

  // R2 reference low watermark is ten stereo frames. Drop before altering decoder/interpolation history.
  if(self.audio.frames.size() > 10) return;
  s16 decoded[4032];
  auto count = decodeSector(decoded);
  if(self.audio.mute || self.audio.muteADPCM) return;  // Decode history advances; reference interpolation pauses.
  bool stereo = drive->sector.data[19] & 1;
  bool halfRate = drive->sector.data[19] & 4;
  resample(decoded, stereo ? count / 2 : count, stereo, halfRate);
}

auto Disc::CDXA::decodeSector(s16* output) -> u32 {
  bool stereo = drive->sector.data[19] & 1;
  bool bits8 = drive->sector.data[19] & 0x10;
  if(!stereo && !bits8) decodeADPCM<false, false>(output);
  if(!stereo &&  bits8) decodeADPCM<false, true >(output);
  if( stereo && !bits8) decodeADPCM<true,  false>(output);
  if( stereo &&  bits8) decodeADPCM<true,  true >(output);
  return 18 * 28 * (bits8 ? 4 : 8);
}

auto Disc::CDXA::pushFrame(s16 left, s16 right) -> void {
  self.audio.pushFrame(left, right, true);
}

auto Disc::CDXA::resample(const s16* input, u32 frames, bool stereo, bool halfRate) -> void {
  u32 position = resamplePosition;
  u32 phase = resampleStep;
  auto interpolate = [&](u32 channel, u32 table) -> s16 {
    s32 sum = 0;
    if(halfRate) {
      // Reference18.9kHz weights; absolute coefficient sum <=56030 keeps this accumulator in32 bits.
      for(u32 tap : range(25)) {
        sum += s32(resampleRing[channel][(position + 32 - 25 + tap) & 31]) * XAInterpolation::half[table][tap];
      }
      sum >>= 15;
    } else {
      for(u32 tap : range(29)) {
        sum += s32(resampleRing[channel][(position + 32 - tap) & 31]) * XAInterpolation::normal[table][tap] >> 15;
      }
    }
    return sclamp<16>(sum);
  };
  auto emit = [&](u32 table) {
    s16 left = interpolate(0, table);
    pushFrame(left, stereo ? interpolate(1, table) : left);
  };

  u32 consumed = 0;
  while(consumed < frames) {
    if(!halfRate || phase >= 7) {
      if(halfRate) {
        phase -= 7;
        position = (position + 1) & 31;
      }
      resampleRing[0][position] = *input++;
      if(stereo) resampleRing[1][position] = *input++;
      consumed++;
      if(!halfRate) {
        position = (position + 1) & 31;
        if(--phase) continue;
        phase = 6;
      }
    }
    if(halfRate) {
      emit(phase);
      phase += 3;
    } else {
      for(u32 table : range(7)) emit(table);
    }
  }
  resamplePosition = position;
  resampleStep = phase;
}

template<bool isStereo, bool is8bit>
auto Disc::CDXA::decodeADPCM(s16* output) -> void {
  constexpr u32 SamplesPerBlock = 28 * (is8bit ? 4 : 8);
  for(u32 block : range(18)) {
    decodeBlock<isStereo, is8bit>(output + block * SamplesPerBlock, 24 + block * 128);
  }
}

template<bool isStereo, bool is8bit>
auto Disc::CDXA::decodeBlock(s16* output, u16 address) -> void {
  static constexpr s32 filterPositive[16] = {0, 60, 115, 98};
  static constexpr s32 filterNegative[16] = {0, 0, -52, -55};
  static constexpr u32 Blocks = is8bit ? 4 : 8;
  static constexpr u32 WordsPerBlock = 28;

  for(u32 block : range(Blocks)) {
    u8  header   = drive->sector.data[address + 4 + block];
    u8  shift    = (header & 0x0f) > 12 ? 9 : (header & 0x0f);
    u8  filter   = header >> 4;
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
      s16 sample = s16(nibble << (is8bit ? 8 : 12)) >> shift;

      s32* previous = isStereo ? &previousSamples[(block & 1) * 2] : &previousSamples[0];
      // Reference rounds each signed predictor product down and saturates decoder history.
      s32 predicted = (previous[0] * positive >> 6) + (previous[1] * negative >> 6);
      s32 interpolated = sclamp<16>(s32(sample) + predicted);
      previous[1] = previous[0];
      previous[0] = interpolated;

      output[index] = interpolated;
      index += isStereo ? 2 : 1;
    }
  }
}
