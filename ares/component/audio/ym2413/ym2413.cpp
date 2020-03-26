#include <ares/ares.hpp>
#include "ym2413.hpp"

namespace ares {

#include "constants.cpp"
#include "operator.cpp"
#include "voice.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto YM2413::clock() -> double {
  auto melody = 0;
  auto rhythm = 0;
  auto mask = -(1 << 12 - 9);

  for(uint index : range(io.rhythmMode ? 6 : 9)) {
    auto& voice = voices[index];
    auto input = voice.modulator.output + voice.modulator.prior >> 8 - voice.feedback;
    voice.modulator.clock(io.clock, voice.modulator.phase >> 9, voice.feedback ? input : 0);
    voice.carrier.clock(io.clock, voice.carrier.phase >> 9, voice.modulator.output << 1);
    voice.carrier.synchronize(true, voice.modulator);
    melody += mask & voice.carrier.output;
  }

  if(io.rhythmMode) {
    auto metallic = [](auto a, auto b) {
      return (a.bit(16) ^ a.bit(11)) | a.bit(12) | b.bit(14) | b.bit(12);
    };
    auto noise = io.noise.bit(0);
    auto m = metallic(hihat.phase, cymbal.phase);
    auto input = bass.modulator.output + bass.modulator.prior >> 8 - bass.feedback;
    bass.modulator.clock(io.clock, bass.modulator.phase >> 9, bass.feedback ? input : 0);
    bass.carrier.clock(io.clock, bass.carrier.phase >> 9, bass.modulator.output << 1);
    bass.carrier.synchronize(true, bass.modulator);
    rhythm += mask & bass.carrier.output;
    rhythm += mask & hihat.clock(io.clock, m << 9 | (m ^ noise ? 52 : 208));
    rhythm += mask & tomtom.clock(io.clock, tomtom.phase >> 9);
    rhythm += mask & snare.clock(io.clock, 1 << snare.phase.bit(17) + 8 ^ noise << 8);
    rhythm += mask & cymbal.clock(io.clock, m << 9 | 256);
    hihat.synchronize(false);
    snare.synchronize(true);
    tomtom.synchronize(true);
    cymbal.synchronize(false);
  }

  auto mix = melody + 2 * rhythm;
  io.noise = io.noise >> 1 ^ io.noise.bit(0) * (1 << 22 | 1 << 8 | 1 << 7 | 1 << 0);
  if(++io.clock == 210 << 12) io.clock -= 210 << 12;
  return mix / 32768.0 * 4.0;
}

auto YM2413::reload(uint4 index) -> void {
  auto& voice = voices[index];
  voice.modulator.audible = io.rhythmMode && index > 6;
  voice.carrier.audible = true;
  if(io.rhythmMode && index >= 6) voice.update(rhythmTones[index - 6]);
  else if(voice.tone) voice.update(melodicTones[voice.tone - 1]);
  else voice.update(customTone);
}

auto YM2413::power(bool isVRC7) -> void {
  if(isVRC7) {
    memory::copy(melodicTones, sizeof(melodicTones), melodicTonesVRC7, sizeof(melodicTonesVRC7));
    memory::copy( rhythmTones, sizeof( rhythmTones),  rhythmTonesVRC7,  sizeof(rhythmTonesVRC7));
  } else {
    memory::copy(melodicTones, sizeof(melodicTones), melodicTonesOPLL, sizeof(melodicTonesOPLL));
    memory::copy( rhythmTones, sizeof( rhythmTones),  rhythmTonesOPLL,  sizeof(rhythmTonesOPLL));
  }
  for(auto& byte : customTone) byte = 0x00;

  if(!sinTable[0] || !expTable[0])
  for(auto x : range(0x100)) {
    //10.9 fixed point phases
    auto y = sin((2 * x + 1) * Math::Pi / 1024.0);   //0x400 units to 2pi radians
    auto z = -(1 << 8) * log(y) / log(2);            //convert to -6 dB/256 units
    auto s = (1 << 10) * pow(2, uint8(~x) / 256.0);  //0x7fa .. 0x400

    sinTable[0x000 + x] = sinTable[0x1ff - x] = uint(z + 0.5) * 2 + 1;
    sinTable[0x200 + x] = sinTable[0x3ff - x] = uint(z + 0.5) * 2 + 0;

    expTable[x * 2 + 1] = +uint(s + 0.5);
    expTable[x * 2 + 0] = ~uint(s + 0.5);
  }

  io = {};
  io.isVRC7 = isVRC7;
  for(uint n : range(9)) {
    voices[n] = {};
    reload(n);
  }

  for(uint n : range(3)) {
    voices[3 * n + 0].modulator.slot = 1 + 6 * n;
    voices[3 * n + 1].modulator.slot = 2 + 6 * n;
    voices[3 * n + 2].modulator.slot = 3 + 6 * n;
    voices[3 * n + 0].carrier.slot   = 4 + 6 * n;
    voices[3 * n + 1].carrier.slot   = 5 + 6 * n;
    voices[3 * n + 2].carrier.slot   = 6 + 6 * n;
  }
}

}
