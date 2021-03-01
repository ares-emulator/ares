//Konami (with Sound Creative Chip)

struct KonamiSCC : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Node::Audio::Stream stream;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    stream = cartridge.node->append<Node::Audio::Stream>("SCC");
    stream->setChannels(1);
    stream->setFrequency(system.colorburst() / 16.0);
    stream->addHighPassFilter(20.0, 1);
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto main() -> void override {
    s32 sample = 256 * 5;

    for(auto& voice : voices) {
      if(voice.frequency <= 8) continue;  //voice is halted when frequency < 9

      voice.clock += 32;
      while(voice.clock > voice.frequency) {
        voice.clock -= voice.frequency + 1;
        voice.counter++;
      }
      sample += voice.wave[voice.counter] * voice.volume * voice.key >> 3;
    }

    stream->frame(mixer[sample] / 32768.0);
    cartridge.step(16);
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x4000 && address <= 0x5fff) data = rom.read(bank[0] << 13 | (n13)address);
    if(address >= 0x6000 && address <= 0x7fff) data = rom.read(bank[1] << 13 | (n13)address);
    if(address >= 0x8000 && address <= 0x9fff) data = rom.read(bank[2] << 13 | (n13)address);
    if(address >= 0xa000 && address <= 0xbfff) data = rom.read(bank[3] << 13 | (n13)address);

    if(address >= 0xc000 && address <= 0xdfff) data = rom.read(bank[0] << 13 | (n13)address);
    if(address >= 0xe000 && address <= 0xffff) data = rom.read(bank[1] << 13 | (n13)address);
    if(address >= 0x0000 && address <= 0x1fff) data = rom.read(bank[2] << 13 | (n13)address);
    if(address >= 0x2000 && address <= 0x3fff) data = rom.read(bank[3] << 13 | (n13)address);

    address.bit(8) = 0;  //SCC ignores A8

    if(address >= 0x9800 && address <= 0x981f) {
      data = voices[0].wave[address & 0x1f];
      if(test.bit(6)) data = voices[0].wave[address + voices[0].counter & 0x1f];
    }

    if(address >= 0x9820 && address <= 0x983f) {
      data = voices[1].wave[address & 0x1f];
      if(test.bit(6)) data = voices[1].wave[address + voices[1].counter & 0x1f];
    }

    if(address >= 0x9840 && address <= 0x985f) {
      data = voices[2].wave[address & 0x1f];
      if(test.bit(6)) data = voices[2].wave[address + voices[2].counter & 0x1f];
    }

    if(address >= 0x9860 && address <= 0x987f) {
      data = voices[3].wave[address & 0x1f];
      if(test.bit(6) || test.bit(7)) data = voices[3].wave[address + voices[3 + test.bit(6)].counter & 0x1f];
    }

    if(address >= 0x9880 && address <= 0x98df) {
      data = 0xff;
    }

    if(address >= 0x98e0 && address <= 0x98ff) {
      //reading the test register sets it to 0xff
      data = test = 0xff;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x5000 && address <= 0x57ff) bank[0] = data;
    if(address >= 0x7000 && address <= 0x77ff) bank[1] = data;
    if(address >= 0x9000 && address <= 0x97ff) bank[2] = data;
    if(address >= 0xb000 && address <= 0xb7ff) bank[3] = data;

    address.bit(8) = 0;  //SCC ignores A8

    if(address >= 0x9800 && address <= 0x981f && !test.bit(6)) {
      voices[0].wave[address & 0x1f] = data;
    }

    if(address >= 0x9820 && address <= 0x983f && !test.bit(6)) {
      voices[1].wave[address & 0x1f] = data;
    }

    if(address >= 0x9840 && address <= 0x985f && !test.bit(6)) {
      voices[2].wave[address & 0x1f] = data;
    }

    if(address >= 0x9860 && address <= 0x987f && !test.bit(6) && !test.bit(7)) {
      voices[3].wave[address & 0x1f] = data;
      voices[4].wave[address & 0x1f] = data;  //shares data with wave channel 3
    }

    if(address == 0x9880) {
      voices[0].frequency.bit(0, 7) = data.bit(0,7);
      if(voices[0].frequency < 9) voices[0].clock = 0;
      if(test.bit(5)) voices[0].clock = 0, voices[0].counter = 0;
    }

    if(address == 0x9881) {
      voices[0].frequency.bit(8,11) = data.bit(0,3);
      if(voices[0].frequency < 9) voices[0].clock = 0;
      if(test.bit(5)) voices[0].clock = 0, voices[0].counter = 0;
    }

    if(address == 0x9882) {
      voices[1].frequency.bit(0, 7) = data.bit(0,7);
      if(voices[1].frequency < 9) voices[1].clock = 0;
      if(test.bit(5)) voices[1].clock = 0, voices[1].counter = 0;
    }

    if(address == 0x9883) {
      voices[1].frequency.bit(8,11) = data.bit(0,3);
      if(voices[1].frequency < 9) voices[1].clock = 0;
      if(test.bit(5)) voices[1].clock = 0, voices[1].counter = 0;
    }

    if(address == 0x9884) {
      voices[2].frequency.bit(0, 7) = data.bit(0,7);
      if(voices[2].frequency < 9) voices[2].clock = 0;
      if(test.bit(5)) voices[2].clock = 0, voices[2].counter = 0;
    }

    if(address == 0x9885) {
      voices[2].frequency.bit(8,11) = data.bit(0,3);
      if(voices[2].frequency < 9) voices[2].clock = 0;
      if(test.bit(5)) voices[2].clock = 0, voices[2].counter = 0;
    }

    if(address == 0x9886) {
      voices[3].frequency.bit(0, 7) = data.bit(0,7);
      if(voices[3].frequency < 9) voices[3].clock = 0;
      if(test.bit(5)) voices[3].clock = 0, voices[3].counter = 0;
    }

    if(address == 0x9887) {
      voices[3].frequency.bit(8,11) = data.bit(0,3);
      if(voices[3].frequency < 9) voices[3].clock = 0;
      if(test.bit(5)) voices[3].clock = 0, voices[3].counter = 0;
    }

    if(address == 0x9888) {
      voices[4].frequency.bit(0, 7) = data.bit(0,7);
      if(voices[4].frequency < 9) voices[4].clock = 0;
      if(test.bit(5)) voices[4].clock = 0, voices[4].counter = 0;
    }

    if(address == 0x9889) {
      voices[4].frequency.bit(8,11) = data.bit(0,3);
      if(voices[4].frequency < 9) voices[4].clock = 0;
      if(test.bit(5)) voices[4].clock = 0, voices[4].counter = 0;
    }

    if(address == 0x988a) {
      voices[0].volume = data.bit(0,3);
    }

    if(address == 0x988b) {
      voices[1].volume = data.bit(0,3);
    }

    if(address == 0x988c) {
      voices[2].volume = data.bit(0,3);
    }

    if(address == 0x988d) {
      voices[3].volume = data.bit(0,3);
    }

    if(address == 0x988e) {
      voices[4].volume = data.bit(0,3);
    }

    if(address == 0x988f) {
      voices[0].key = data.bit(0);
      voices[1].key = data.bit(1);
      voices[2].key = data.bit(2);
      voices[3].key = data.bit(3);
      voices[4].key = data.bit(4);
    }

    if(address >= 0x98e0 && address <= 0x98ff) test = data;
  }

  auto power() -> void override {
    bank[0] = 0;
    bank[1] = 1;
    bank[2] = 2;
    bank[3] = 3;
    voices[0] = {};
    voices[1] = {};
    voices[2] = {};
    voices[3] = {};
    voices[4] = {};
    test = 0;

    mixer.resize(512 * 5);
    auto table = mixer.data() + 256 * 5;
    for(s32 n : range(256 * 5)) {
      s16 volume = n * 8 * 16 / 5;
      table[+n] = +volume;
      table[-n] = -volume;
    }
  }

  auto serialize(serializer& s) -> void override {
    s(bank);
    for(auto& voice : voices) {
      s(voice.clock);
      s(voice.frequency);
      s(voice.counter);
      s(voice.volume);
      s(voice.key);
      s(voice.wave);
    }
    s(test);
  }

  n8 bank[4];

  struct Voice {
    n16 clock;
    n12 frequency;
    n5  counter;
    n4  volume;
    n1  key;
    i8  wave[32];
  } voices[5];

  n8 test;

//unserialized:
  vector<i16> mixer;
};
