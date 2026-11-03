//the Bung Pocket Voice pairs a flash cartridge with a one bit voice recorder.
//its ASIC splits the ROM into a 24KB fixed window and an 8KB banked window,
//both displaced by a 32KB granular offset:
//
//  write 0x0000  a value other than 0x0a hides SRAM and exposes the sound chip
//  write 0x6000  low byte of the 8KB bank at 0x6000-0x7fff
//  write 0x7000  high two bits of that bank, giving a range of 0x000-0x3ff
//  write 0xa003  32KB offset added to both windows, so 0x00-0xff spans 8MB
//
//32KB programs are stored one per offset: the fixed window covers 0x0000-0x5fff
//of the program and bank offset*4+3 supplies its final 8KB.
//
//  read  0xa000  status; d3 set when a sample may be exchanged
//  write 0xa000  d0 play, d1 record, d2 end of play, d6 flash write, d7 enable
//  rw    0xa001  sample port, one byte per 330us
//
//the sound chip is a bitstream device: each byte is eight output levels, LSB
//first, that the speaker demodulates directly. Bung's documentation calls it
//only a "one bit bit stream"; the firmware's own voice data confirms levels
//over delta modulation, whose integral would run away rather than stay
//bounded. Flash programming is recognised at 0xa000 d6 but not emulated.
//
//unverified against real hardware: bit order, the 3000Hz filter corner and
//its order are judged by ear, not measured. The mapper is on firmer ground,
//checked against what the firmware itself does.

struct PocketVoice : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Node::Audio::Stream stream;

  static constexpr u32 ByteRate = 3030;         //330us per byte
  static constexpr u32 BitRate  = ByteRate * 8;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    stream = cartridge.node->append<Node::Audio::Stream>("Pocket Voice");
    stream->setChannels(1);
    stream->setFrequency(BitRate);
    //the speaker is what demodulates the stream. a one pole roll-off is far too
    //gentle for that: at this rate it still passes most of the quantisation
    //noise a one bit stream carries, which is audible as hiss over the voice.
    //a fourth order butterworth leaves the 300-3400Hz band alone and puts the
    //noise 19dB further down. the high pass takes the DC an unbalanced run of
    //bits implies, which the speaker could not hold either
    stream->addLowPassFilter(3000.0, 2, 2);
    stream->addHighPassFilter(20.0, 1);
  }

  auto unload() -> void override {
    cartridge.node->remove(stream);
    stream.reset();
  }

  auto main() -> void override {
    //one iteration is one byte period of the sound chip
    if(io.play) {
      //each bit drives the speaker high or low; the filters reconstruct it.
      //least significant bit first: reversing a byte leaves its pulse density
      //untouched, so this is audibly clearer rather than measurably so
      for(u32 n : range(8)) stream->frame(io.sample.bit(n) ? +1.0 : -1.0);
      //ask for the next byte; if the program is late this one plays again
      io.ready = 1;
    } else {
      if(io.record) {
        //no microphone is attached: half density is the silent level
        io.sample = 0x55;
        io.ready = 1;
      }
      for(u32 n : range(8)) stream->frame(0.0);
    }

    step(cartridge.frequency() / ByteRate);
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x5fff) {
      return romRead(io.offset * 0x8000 + address);
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      return romRead(io.offset * 0x8000 + io.bank * 0x2000 + (address & 0x1fff));
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      //the sound chip only answers once SRAM decoding has been switched off
      if(io.ramEnable) return 0xff;

      if(address == 0xa000) {
        return io.ready << 3;
      }

      if(address == 0xa001) {
        if(!io.record) return 0xff;
        io.ready = 0;
        return io.sample;
      }

      return 0xff;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      io.ramEnable = data.bit(0,3) == 0x0a;
      return;
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      //with flash programming enabled these are commands to the flash chip
      if(io.flashEnable) return;
      if(address <= 0x6fff) io.bank.bit(0,7) = data.bit(0,7);
      if(address >= 0x7000) io.bank.bit(8,9) = data.bit(0,1);
      return;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      if(io.ramEnable) return;

      if(address == 0xa000) {
        io.play = data.bit(0);
        io.record = data.bit(1);
        io.flashEnable = data.bit(6);
        if(!io.play && !io.record) io.ready = 0;
        return;
      }

      if(address == 0xa001) {
        if(!io.play) return;
        io.sample = data;
        io.ready = 0;
        return;
      }

      if(address == 0xa003) {
        io.offset = data;
        return;
      }
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(io.ramEnable);
    s(io.flashEnable);
    s(io.bank);
    s(io.offset);
    s(io.play);
    s(io.record);
    s(io.ready);
    s(io.sample);
  }

private:
  //the offset register spans 8MB while the fitted flash is far smaller, so the
  //space above it reads open bus rather than aliasing back over the ROM
  auto romRead(u32 offset) -> n8 {
    if(offset >= rom.size()) return 0xff;
    return rom.read(offset);
  }

  struct IO {
    n1  ramEnable;
    n1  flashEnable;
    n10 bank = 0x003;
    n8  offset;
    n1  play;
    n1  record;
    n1  ready;
    n8  sample;
  } io;
};
