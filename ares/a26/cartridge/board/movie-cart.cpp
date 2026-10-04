struct MovieCart : Interface {
  using Interface::Interface;

  static constexpr u32 TitleCycles = 1'000'000;
  static constexpr u16 StreamRoutine = 0x1080;
  static constexpr u16 BlankRoutine = 0x1180;
  static constexpr u16 InputRoutine = 0x1100;
  static constexpr u16 DifficultyRoutine = 0x1200;
  static constexpr u16 ConsoleRoutine = 0x1300;
  static constexpr u16 PulseRoutine = 0x1560;
  static constexpr u16 LineTrampoline = 0x1070;

  struct Kernel {
    std::array<u8, 1_KiB> image{};
    u16 vsyncLines = 0;
    u16 vblankLines = 0;
    u16 overscanLines = 0;
    u16 visibleDone = 0;
    u16 visibleExit = 0;
    u16 background = 0;
    std::array<u16, 5> playfieldColor{};
    u16 playfield0Left = 0;
    u16 playfield1Left = 0;
    u16 playfield2Left = 0;
    u16 playfield0Right = 0;
    u16 playfield1Right = 0;
    u16 playfield2Right = 0;
    std::array<u16, 2> audioVolume{};
  } kernel;

  VFS::File stream;
  std::array<std::array<u8, MVC::FieldSize>, 2> fields{};
  MVC::FieldDescriptor descriptors[2];
  std::array<u32, 2> loadedFields = {~0u, ~0u};
  std::array<bool, 2> blankFields{};
  std::array<u8, 32> fingerprint{};
  u64 streamSize = 0;
  u32 fieldCount = 0;
  u32 fieldNumber = 0;
  u16 scanline = 0;
  n1 activeBuffer = 0;
  n1 odd = 1;
  n1 a7 = 0;
  n1 a10 = 0;
  n16 a10Pulses = 0;
  bool streamStarted = false;
  u8 previousCommand = 0;
  u8 repeatFields = 0;
  u8 seekSpeed = 2;
  u8 muteFields = 0;
  u8 volume = 15;
  u8 brightness = 15;
  u8 osdMode = 0;
  bool monochrome = false;
  bool playing = true;
  enum class TitleState : u8 { Display, Exiting, Stream } titleState = TitleState::Display;
  u32 titleCycles = 0;
  bool ready = false;

  auto emit(u16& pc, u8 value) -> void {
    kernel.image[pc++ & 0x03ff] = value;
  }

  auto immediate(u16& pc, u8 opcode, u8 value, u16* operand = nullptr) -> void {
    emit(pc, opcode);
    if(operand) *operand = pc;
    emit(pc, value);
  }

  auto absolute(u16& pc, u8 opcode, u16 address) -> void {
    emit(pc, opcode);
    emit(pc, address);
    emit(pc, address >> 8);
  }

  auto zeroPage(u16& pc, u8 opcode, u8 address) -> void {
    emit(pc, opcode);
    emit(pc, address);
  }

  auto branch(u16& pc, u8 opcode, u16 target) -> void {
    emit(pc, opcode);
    auto origin = pc + 1;
    emit(pc, (u8)(target - origin));
  }

  auto branchPlaceholder(u16& pc, u8 opcode) -> u16 {
    emit(pc, opcode);
    auto operand = pc;
    emit(pc, 0);
    return operand;
  }

  auto patchBranch(u16 operand, u16 target) -> void {
    kernel.image[operand & 0x03ff] = (u8)(target - (operand + 1));
  }

  auto buildKernel() -> void {
    kernel = {};
    kernel.image.fill(0xea);
    u16 pc = 0;

    emit(pc, 0x78);                              //SEI
    emit(pc, 0xd8);                              //CLD
    immediate(pc, 0xa2, 0xff);                   //LDX #$ff
    emit(pc, 0x9a);                              //TXS
    immediate(pc, 0xa9, 0x02);                   //LDA #$02
    zeroPage(pc, 0x85, 0x01);                    //STA VBLANK
    immediate(pc, 0xa9, 0x00);                   //LDA #$00
    zeroPage(pc, 0x85, 0x0a);                    //STA CTRLPF
    immediate(pc, 0xa9, 0x04);                   //LDA #pure tone
    zeroPage(pc, 0x85, 0x15);                    //STA AUDC0
    immediate(pc, 0xa9, 0x00);                   //LDA #frequency
    zeroPage(pc, 0x85, 0x17);                    //STA AUDF0

    auto frame = pc;
    immediate(pc, 0xa9, 0x02);                   //LDA #$02
    zeroPage(pc, 0x85, 0x00);                    //STA VSYNC
    immediate(pc, 0xa2, 3, &kernel.vsyncLines);  //LDX #vsync
    auto vsyncLoop = pc;
    absolute(pc, 0x20, BlankRoutine);            //JSR blank line
    emit(pc, 0xca);                              //DEX
    branch(pc, 0xd0, vsyncLoop);                 //BNE
    immediate(pc, 0xa9, 0x00);                   //LDA #0
    zeroPage(pc, 0x85, 0x00);                    //STA VSYNC
    immediate(pc, 0xa2, 37, &kernel.vblankLines);//LDX #vblank
    auto vblankLoop = pc;
    absolute(pc, 0x20, BlankRoutine);            //JSR blank line
    emit(pc, 0xca);                              //DEX
    branch(pc, 0xd0, vblankLoop);                //BNE
    immediate(pc, 0xa9, 0x00);                   //LDA #0
    zeroPage(pc, 0x85, 0x01);                    //STA VBLANK
    absolute(pc, 0x4c, StreamRoutine);            //JMP visible stream
    auto visibleDone = pc;
    kernel.visibleExit = visibleDone + 0x1000;
    immediate(pc, 0xa9, 0x02);                   //LDA #2
    zeroPage(pc, 0x85, 0x01);                    //STA VBLANK
    absolute(pc, 0x20, InputRoutine);             //JSR input sampler
    immediate(pc, 0xa2, 30, &kernel.overscanLines);//LDX #overscan
    auto overscanLoop = pc;
    absolute(pc, 0x20, BlankRoutine);            //JSR blank line
    emit(pc, 0xca);                              //DEX
    branch(pc, 0xd0, overscanLoop);               //BNE
    absolute(pc, 0x4c, 0x1000 + frame);           //JMP frame

    pc = LineTrampoline & 0x03ff;
    emit(pc, 0x4c);                              //JMP visible stream/done
    kernel.visibleDone = pc;
    emit(pc, StreamRoutine & 0xff);
    emit(pc, StreamRoutine >> 8);

    pc = PulseRoutine & 0x03ff;
    emit(pc, 0x60);                              //RTS (one A10 pulse)

    pc = StreamRoutine & 0x03ff;
    zeroPage(pc, 0x85, 0x02);                    //STA WSYNC
    immediate(pc, 0xa9, 0x00, &kernel.background);
    zeroPage(pc, 0x85, 0x09);                    //STA COLUBK
    immediate(pc, 0xa9, 0x0e, &kernel.playfieldColor[0]);
    zeroPage(pc, 0x85, 0x08);                    //STA COLUPF
    immediate(pc, 0xa9, 0x00, &kernel.playfield0Left);
    zeroPage(pc, 0x85, 0x0d);                    //STA PF0
    immediate(pc, 0xa9, 0x00, &kernel.playfield1Left);
    zeroPage(pc, 0x85, 0x0e);                    //STA PF1
    immediate(pc, 0xa9, 0x00, &kernel.playfield2Left);
    zeroPage(pc, 0x85, 0x0f);                    //STA PF2
    immediate(pc, 0xa9, 0x00, &kernel.audioVolume[0]);
    zeroPage(pc, 0x85, 0x19);                    //STA AUDV0
    emit(pc, 0xea);                              //align five color cells
    immediate(pc, 0xa9, 0x0e, &kernel.playfieldColor[1]);
    zeroPage(pc, 0x85, 0x08);                    //STA COLUPF
    immediate(pc, 0xa9, 0x0e, &kernel.playfieldColor[2]);
    zeroPage(pc, 0x85, 0x08);                    //STA COLUPF
    immediate(pc, 0xa9, 0x00, &kernel.playfield0Right);
    zeroPage(pc, 0x85, 0x0d);                    //STA PF0
    immediate(pc, 0xa9, 0x0e, &kernel.playfieldColor[3]);
    zeroPage(pc, 0x85, 0x08);                    //STA COLUPF
    immediate(pc, 0xa9, 0x00, &kernel.playfield1Right);
    zeroPage(pc, 0x85, 0x0e);                    //STA PF1
    immediate(pc, 0xa9, 0x0e, &kernel.playfieldColor[4]);
    zeroPage(pc, 0x85, 0x08);                    //STA COLUPF
    immediate(pc, 0xa9, 0x00, &kernel.playfield2Right);
    zeroPage(pc, 0x85, 0x0f);                    //STA PF2
    absolute(pc, 0x4c, LineTrampoline);           //JMP low-page trampoline

    pc = BlankRoutine & 0x03ff;
    zeroPage(pc, 0x85, 0x02);                    //STA WSYNC
    immediate(pc, 0xa9, 0x00, &kernel.audioVolume[1]);
    zeroPage(pc, 0x85, 0x19);                    //STA AUDV0
    emit(pc, 0x60);                              //RTS

    pc = InputRoutine & 0x03ff;
    zeroPage(pc, 0xa5, 0x0c);                    //LDA INPT4
    auto noFire = branchPlaceholder(pc, 0x30);   //BMI not pressed
    absolute(pc, 0x20, PulseRoutine);
    emit(pc, 0x60);                              //fire = command 1
    patchBranch(noFire, pc);

    auto joystickCommand = [&](u8 mask, u8 pulses) {
      absolute(pc, 0xad, 0x0280);                //LDA SWCHA
      immediate(pc, 0x29, mask);                 //AND direction
      auto notPressed = branchPlaceholder(pc, 0xd0);
      for(u32 pulse : range(pulses)) absolute(pc, 0x20, PulseRoutine);
      emit(pc, 0x60);
      patchBranch(notPressed, pc);
    };
    joystickCommand(0x40, 2);                    //left = seek backward
    joystickCommand(0x80, 3);                    //right = seek forward
    joystickCommand(0x10, 4);                    //up = volume up
    joystickCommand(0x20, 5);                    //down = volume down
    absolute(pc, 0x4c, DifficultyRoutine);        //JMP difficulty sampler

    pc = DifficultyRoutine & 0x03ff;
    auto difficultyCommand = [&](u8 mask, u8 pulses) {
      absolute(pc, 0xad, 0x0282);                //LDA SWCHB
      immediate(pc, 0x29, mask);                 //AND difficulty switch
      auto inactive = branchPlaceholder(pc, 0xd0);
      for(u32 pulse : range(pulses)) absolute(pc, 0x20, PulseRoutine);
      emit(pc, 0x60);
      patchBranch(inactive, pc);
    };
    difficultyCommand(0x40, 9);                  //left difficulty = ten seconds back
    difficultyCommand(0x80, 10);                 //right difficulty = single step
    absolute(pc, 0x4c, ConsoleRoutine);           //JMP console sampler

    pc = ConsoleRoutine & 0x03ff;
    auto consoleCommand = [&](u8 mask, u8 pulses) {
      absolute(pc, 0xad, 0x0282);                //LDA SWCHB
      immediate(pc, 0x29, mask);                 //AND console switch
      auto inactive = branchPlaceholder(pc, 0xd0);
      for(u32 pulse : range(pulses)) absolute(pc, 0x20, PulseRoutine);
      emit(pc, 0x60);
      patchBranch(inactive, pc);
    };
    consoleCommand(0x02, 6);                     //select = cycle OSD
    consoleCommand(0x01, 7);                     //reset = restart movie
    consoleCommand(0x08, 8);                     //B/W = monochrome toggle
    emit(pc, 0x60);

    kernel.image[0x03fc] = 0x00;
    kernel.image[0x03fd] = 0x10;
    kernel.image[0x03fe] = 0x00;
    kernel.image[0x03ff] = 0x10;
  }

  auto fail(string reason) -> void {
    ready = false;
    cartridge.node->setAttribute("error", reason);
  }

  auto updateStatus() -> void {
    cartridge.node->setAttribute("moviecart-field", string{fieldNumber});
    cartridge.node->setAttribute("moviecart-state", playing ? "playing" : "paused");
    cartridge.node->setAttribute("moviecart-volume", string{volume});
    cartridge.node->setAttribute("moviecart-brightness", string{brightness});
    cartridge.node->setAttribute("moviecart-monochrome", monochrome ? "on" : "off");
    cartridge.node->setAttribute("moviecart-osd", string{osdMode});
    cartridge.node->setAttribute("moviecart-seek-speed", string{seekSpeed});
    cartridge.node->setAttribute("moviecart-repeat", string{repeatFields});
    cartridge.node->setAttribute("moviecart-mute", string{muteFields});
    static constexpr const char* titleNames[] = {"display", "exiting", "stream"};
    cartridge.node->setAttribute("moviecart-title", titleNames[(u32)titleState]);
  }

  auto readField(u32 number, u32 buffer) -> bool {
    if(!stream || number >= fieldCount || buffer >= fields.size()) return false;
    auto offset = (u64)number * MVC::FieldSize;
    if(offset > streamSize || MVC::FieldSize > streamSize - offset) return false;
    std::array<u8, MVC::FieldSize> candidate{};
    stream->seek(offset);
    stream->read({candidate.data(), candidate.size()});
    MVC::FieldDescriptor descriptor;
    if(!MVC::parseField(candidate, descriptor)) {
      loadedFields[buffer] = number;
      blankFields[buffer] = true;
      return false;
    }
    fields[buffer] = candidate;
    descriptors[buffer] = descriptor;
    loadedFields[buffer] = number;
    blankFields[buffer] = false;
    return true;
  }

  auto calculateFingerprint() -> void {
    Hash::SHA256 hash;
    MVC::fingerprintPrefix(hash, streamSize, fields[0], fields[1]);
    stream->seek(streamSize - MVC::FieldSize);
    for(u32 byte : range(MVC::FieldSize)) hash.input(stream->read());
    auto output = hash.output();
    for(u32 index : range(fingerprint.size())) fingerprint[index] = output[index];
  }

  auto load() -> void override {
    cartridge.node->setAttribute("error");
    stream = pak->read("stream.mvc");
    if(!stream) return fail("MovieCart package is missing stream.mvc");
    streamSize = stream->size();
    if(streamSize < MVC::MinimumSize || streamSize % MVC::FieldSize) return fail("invalid MovieCart stream size");
    if(streamSize / MVC::FieldSize > 0xffffffffull) return fail("MovieCart stream has too many fields");
    fieldCount = streamSize / MVC::FieldSize;
    if(!readField(0, 0) || !readField(1, 1)) return fail("invalid initial MovieCart field");
    calculateFingerprint();
    ready = true;
  }

  auto unload() -> void override {
    stream.reset();
    for(auto& field : fields) field.fill(0);
    descriptors[0] = {};
    descriptors[1] = {};
    loadedFields = {~0u, ~0u};
    blankFields = {};
    fingerprint.fill(0);
    streamSize = 0;
    fieldCount = 0;
    ready = false;
  }

  auto activateField() -> void {
    auto& descriptor = descriptors[(u32)activeBuffer];
    scanline = 0;
    if(!descriptor.valid) return;
    odd = descriptor.embeddedFrame & 1;
    kernel.image[kernel.vsyncLines] = descriptor.vsync;
    kernel.image[kernel.vblankLines] = descriptor.vblank;
    kernel.image[kernel.overscanLines] = descriptor.overscan;
    kernel.image[kernel.visibleDone + 0] = StreamRoutine & 0xff;
    kernel.image[kernel.visibleDone + 1] = StreamRoutine >> 8;
    if(blankFields[(u32)activeBuffer]) {
      cartridge.node->setAttribute("error", string{"invalid MovieCart field ", fieldNumber});
    } else {
      cartridge.node->setAttribute("error");
    }
    patchLeft();
  }

  auto clearLeft() -> void {
    kernel.image[kernel.background] = 0;
    for(u32 cell : range(3)) kernel.image[kernel.playfieldColor[cell]] = 0;
    kernel.image[kernel.playfield0Left] = 0;
    kernel.image[kernel.playfield1Left] = 0;
    kernel.image[kernel.playfield2Left] = 0;
  }

  auto clearRight() -> void {
    for(u32 cell : range(3, 5)) kernel.image[kernel.playfieldColor[cell]] = 0;
    kernel.image[kernel.playfield0Right] = 0;
    kernel.image[kernel.playfield1Right] = 0;
    kernel.image[kernel.playfield2Right] = 0;
  }

  static auto reverseByte(u8 value) -> u8 {
    value = (value & 0x55) << 1 | (value & 0xaa) >> 1;
    value = (value & 0x33) << 2 | (value & 0xcc) >> 2;
    return value << 4 | value >> 4;
  }

  static auto reverseNybble(u8 value) -> u8 {
    value &= 15;
    return ((value & 1) << 3) | ((value & 2) << 1) | ((value & 4) >> 1) | ((value & 8) >> 3);
  }

  auto adjustColor(u8 color) const -> u8 {
    auto luminance = ((color & 0x0e) * brightness / 15) & 0x0e;
    return (monochrome ? 0 : color & 0xf0) | luminance;
  }

  static auto titlePixel(u32 x, u32 y) -> bool {
    auto letterM = (x == 2 || x == 11) || (y < 5 && (x == 2 + y || x == 11 - y));
    auto letterV = x == 15 + y / 2 || x == 24 - y / 2;
    auto letterC = (x == 28 && y > 0 && y < 11) || ((y == 0 || y == 11) && x >= 29 && x <= 37);
    return letterM || letterV || letterC;
  }

  auto titleByte(u32 visibleLine, u32 cell, u32 visibleLines) const -> u8 {
    static constexpr u32 height = 12;
    auto presentationLines = Region::NTSC() ? 192u : 228u;
    auto top = (presentationLines - height) / 2;
    if(visibleLines < top + height) top = visibleLines > height ? visibleLines - height : 0;
    if(visibleLine < top || visibleLine >= top + height) return 0;
    auto y = visibleLine - top;
    u8 output = 0;
    for(u32 bit : range(8)) if(titlePixel(cell * 8 + bit, y)) output |= 0x80 >> bit;
    return output;
  }

  auto patchLeft() -> void {
    auto buffer = (u32)activeBuffer;
    auto& descriptor = descriptors[buffer];
    if(blankFields[buffer] || !descriptor.valid || scanline >= descriptor.audioCount) {
      clearLeft();
      for(auto operand : kernel.audioVolume) kernel.image[operand] = 0;
      return;
    }

    auto visibleStart = (u32)descriptor.vsync + descriptor.vblank;
    if(titleState != TitleState::Stream) {
      for(auto operand : kernel.audioVolume) kernel.image[operand] = 0;
      if(scanline < visibleStart || scanline >= visibleStart + descriptor.visible) return clearLeft();
      auto visibleLine = scanline - visibleStart;
      auto graph0 = titleByte(visibleLine, 0, descriptor.visible);
      auto graph1 = titleByte(visibleLine, 1, descriptor.visible);
      auto graph2 = titleByte(visibleLine, 2, descriptor.visible);
      kernel.image[kernel.background] = adjustColor(((visibleLine / 12 + 1) & 15) << 4 | 0x02);
      for(u32 cell : range(3)) kernel.image[kernel.playfieldColor[cell]] = adjustColor(0x0e);
      kernel.image[kernel.playfield0Left] = reverseNybble(graph0 >> 4) << 4;
      kernel.image[kernel.playfield1Left] = graph0 << 4 | graph1 >> 4;
      kernel.image[kernel.playfield2Left] = reverseByte(graph1 << 4 | graph2 >> 4);
      return;
    }

    auto& field = fields[buffer];
    auto audio = muteFields ? 0 : (field[descriptor.audioOffset + scanline] & 15) * volume / 15;
    for(auto operand : kernel.audioVolume) kernel.image[operand] = audio;
    if(scanline < visibleStart || scanline >= visibleStart + descriptor.visible) {
      clearLeft();
      return;
    }

    auto visibleLine = scanline - visibleStart;
    auto overlay = osdMode && visibleLine < descriptor.timecodeCount / 5;
    auto graph = overlay ? descriptor.timecodeOffset + visibleLine * 5
                         : descriptor.graphOffset + visibleLine * 5;
    auto color = descriptor.colorOffset + visibleLine * 5;
    auto backgroundLine = min<u32>(visibleLine + !odd, descriptor.backgroundCount - 1);
    auto graph0 = field[graph + 0];
    auto graph1 = field[graph + 1];
    auto graph2 = field[graph + 2];
    kernel.image[kernel.background] = overlay ? 0 : adjustColor(field[descriptor.backgroundOffset + backgroundLine]);
    for(u32 cell : range(3)) {
      kernel.image[kernel.playfieldColor[cell]] = overlay ? adjustColor(0x0e) : adjustColor(field[color + cell]);
    }
    kernel.image[kernel.playfield0Left] = reverseNybble(graph0 >> 4) << 4;
    kernel.image[kernel.playfield1Left] = graph0 << 4 | graph1 >> 4;
    kernel.image[kernel.playfield2Left] = reverseByte(graph1 << 4 | graph2 >> 4);
  }

  auto patchRight() -> void {
    auto buffer = (u32)activeBuffer;
    auto& descriptor = descriptors[buffer];
    auto visibleStart = (u32)descriptor.vsync + descriptor.vblank;
    if(blankFields[buffer] || !descriptor.valid || scanline < visibleStart
      || scanline >= visibleStart + descriptor.visible) {
      clearRight();
      return;
    }
    if(titleState != TitleState::Stream) {
      auto visibleLine = scanline - visibleStart;
      auto graph2 = titleByte(visibleLine, 2, descriptor.visible);
      kernel.image[kernel.playfield0Right] = reverseNybble(graph2) << 4;
      kernel.image[kernel.playfield1Right] = titleByte(visibleLine, 3, descriptor.visible);
      kernel.image[kernel.playfield2Right] = reverseByte(titleByte(visibleLine, 4, descriptor.visible));
      kernel.image[kernel.playfieldColor[3]] = adjustColor(0x0e);
      kernel.image[kernel.playfieldColor[4]] = adjustColor(0x0e);
      return;
    }
    auto& field = fields[buffer];
    auto visibleLine = scanline - visibleStart;
    auto overlay = osdMode && visibleLine < descriptor.timecodeCount / 5;
    auto graph = overlay ? descriptor.timecodeOffset + visibleLine * 5
                         : descriptor.graphOffset + visibleLine * 5;
    auto color = descriptor.colorOffset + visibleLine * 5;
    auto graph2 = field[graph + 2];
    kernel.image[kernel.playfield0Right] = reverseNybble(graph2) << 4;
    kernel.image[kernel.playfield1Right] = field[graph + 3];
    kernel.image[kernel.playfield2Right] = reverseByte(field[graph + 4]);
    kernel.image[kernel.playfieldColor[3]] = overlay ? adjustColor(0x0e) : adjustColor(field[color + 3]);
    kernel.image[kernel.playfieldColor[4]] = overlay ? adjustColor(0x0e) : adjustColor(field[color + 4]);
  }

  auto advanceField() -> void {
    if(titleState != TitleState::Stream) {
      a10Pulses = 0;
      previousCommand = 0;
      fieldNumber = 0;
      activeBuffer = loadedFields[0] == 0 ? 0 : 1;
      activateField();
      updateStatus();
      return;
    }
    auto command = min<u32>(a10Pulses, 10);
    a10Pulses = 0;
    if(muteFields) muteFields--;
    s32 seek = 0;
    bool restart = false;
    bool step = false;
    auto fresh = command != previousCommand;
    if(command == 2 || command == 3) {
      if(fresh) {
        repeatFields = 0;
        seekSpeed = 2;
      } else {
        if(repeatFields < 255) repeatFields++;
        if(repeatFields && repeatFields % 30 == 0) seekSpeed = min<u32>(seekSpeed * 2, 64);
      }
      seek = command == 2 ? -(s32)seekSpeed : +(s32)seekSpeed;
    } else {
      repeatFields = 0;
      seekSpeed = 2;
    }
    if(fresh) {
      if(command == 1) playing = !playing;
      if(command == 4) {
        if(osdMode == 2 && brightness < 15) brightness++;
        else if(osdMode != 2 && volume < 15) volume++;
      }
      if(command == 5) {
        if(osdMode == 2 && brightness > 0) brightness--;
        else if(osdMode != 2 && volume > 0) volume--;
      }
      if(command == 6) osdMode = (osdMode + 1) % 3;
      if(command == 7) {
        restart = true;
        playing = true;
      }
      if(command == 8) monochrome = !monochrome;
      if(command == 9) seek = -(s32)descriptors[(u32)activeBuffer].rate * 10;
      if(command == 10) {
        playing = false;
        step = true;
      }
    }
    previousCommand = command;

    s64 requested = fieldNumber;
    if(restart) requested = 0;
    else if(seek) requested += seek;
    else if(step) requested++;
    else if(playing) requested++;
    while(requested < 0) requested += 2;
    auto reachedEOF = requested >= fieldCount;
    while(requested >= fieldCount) requested -= 2;
    if(reachedEOF) {
      repeatFields = 0;
      seekSpeed = 2;
      muteFields = 2;
    }
    auto next = (u32)requested;
    auto targetBuffer = next == fieldNumber ? (u32)activeBuffer : (u32)!activeBuffer;
    if(loadedFields[targetBuffer] != next) readField(next, targetBuffer);
    fieldNumber = next;
    activeBuffer = targetBuffer;
    auto lookahead = fieldNumber + 1;
    if(lookahead >= fieldCount) lookahead = fieldCount - 2;
    auto lookaheadBuffer = (u32)!activeBuffer;
    if(loadedFields[lookaheadBuffer] != lookahead) readField(lookahead, lookaheadBuffer);
    activateField();
    updateStatus();
  }

  auto process(n16 address) -> void {
    if(titleState == TitleState::Display && ++titleCycles >= TitleCycles) titleState = TitleState::Exiting;
    auto nextA10 = address.bit(10);
    if(!a10 && nextA10 && !address.bit(7)) a10Pulses++;
    a10 = nextA10;
    auto nextA7 = address.bit(7);
    if(!streamStarted) {
      auto routine = address & 0x03ff;
      if(nextA7 && (routine == (StreamRoutine & 0x03ff) || routine == (BlankRoutine & 0x03ff))) {
        streamStarted = true;
        patchRight();
      }
      a7 = nextA7;
      return;
    }
    if(titleState == TitleState::Exiting && !a7 && nextA7) {
      titleState = TitleState::Stream;
      fieldNumber = 0;
      activeBuffer = loadedFields[0] == 0 ? 0 : 1;
      activateField();
      updateStatus();
    }
    if(!a7 && nextA7) patchRight();
    if(a7 && !nextA7) {
      scanline++;
      auto& descriptor = descriptors[(u32)activeBuffer];
      auto totalLines = (u32)descriptor.vsync + descriptor.vblank + descriptor.visible + descriptor.overscan;
      if(!descriptor.valid || scanline >= totalLines) advanceField();
      else {
        patchLeft();
        auto visibleEnd = (u32)descriptor.vsync + descriptor.vblank + descriptor.visible;
        if(scanline == visibleEnd) {
          kernel.image[kernel.visibleDone + 0] = kernel.visibleExit & 0xff;
          kernel.image[kernel.visibleDone + 1] = kernel.visibleExit >> 8;
        }
      }
    }
    a7 = nextA7;
  }

  auto power(bool) -> void override {
    if(!ready) return;
    buildKernel();
    if(!readField(0, 0) || !readField(1, 1)) return fail("MovieCart stream changed while attached");
    fieldNumber = 0;
    activeBuffer = 0;
    odd = 0;
    a7 = 0;
    a10 = 0;
    a10Pulses = 0;
    streamStarted = false;
    previousCommand = 0;
    repeatFields = 0;
    seekSpeed = 2;
    muteFields = 0;
    volume = 15;
    brightness = 15;
    osdMode = 0;
    monochrome = false;
    playing = true;
    titleState = TitleState::Display;
    titleCycles = 0;
    activateField();
    updateStatus();
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!ready || !address.bit(12)) return data;
    process(address);
    return kernel.image[address & 0x03ff];
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(ready && address.bit(12)) process(address);
    return data;
  }

  auto serializeDescriptor(serializer& s, MVC::FieldDescriptor& descriptor) -> void {
    s(descriptor.valid);
    s(descriptor.extended);
    s(descriptor.signatureOffset);
    s(descriptor.vsync);
    s(descriptor.vblank);
    s(descriptor.overscan);
    s(descriptor.visible);
    s(descriptor.rate);
    s(descriptor.embeddedFrame);
    s(descriptor.audioOffset);
    s(descriptor.audioCount);
    s(descriptor.graphOffset);
    s(descriptor.graphCount);
    s(descriptor.colorOffset);
    s(descriptor.colorCount);
    s(descriptor.backgroundOffset);
    s(descriptor.backgroundCount);
    s(descriptor.timecodeOffset);
    s(descriptor.timecodeCount);
  }

  auto serialize(serializer& s) -> void override {
    if(!ready) return;
    auto savedSize = streamSize;
    auto savedFingerprint = fingerprint;
    s(std::span<u8>{kernel.image.data(), kernel.image.size()});
    for(auto& field : fields) s(std::span<u8>{field.data(), field.size()});
    serializeDescriptor(s, descriptors[0]);
    serializeDescriptor(s, descriptors[1]);
    for(auto& loadedField : loadedFields) s(loadedField);
    for(auto& blankField : blankFields) s(blankField);
    s(std::span<u8>{savedFingerprint.data(), savedFingerprint.size()});
    s(savedSize);
    s(fieldNumber);
    s(scanline);
    s(activeBuffer);
    s(odd);
    s(a7);
    s(a10);
    s(a10Pulses);
    s(streamStarted);
    s(previousCommand);
    s(repeatFields);
    s(seekSpeed);
    s(muteFields);
    s(volume);
    s(brightness);
    s(osdMode);
    s(monochrome);
    s(playing);
    s(titleState);
    s(titleCycles);
    if(s.reading()) {
      if(savedSize != streamSize || savedFingerprint != fingerprint) return fail("MovieCart state media mismatch");
      MVC::FieldDescriptor parsed[2];
      if(!MVC::parseField(fields[0], parsed[0]) || !MVC::parseField(fields[1], parsed[1])
        || parsed[0] != descriptors[0] || parsed[1] != descriptors[1]) {
        return fail("invalid MovieCart field state");
      }
      auto& descriptor = descriptors[(u32)activeBuffer];
      auto totalLines = (u32)descriptor.vsync + descriptor.vblank + descriptor.visible + descriptor.overscan;
      if(fieldNumber >= fieldCount || loadedFields[0] >= fieldCount || loadedFields[1] >= fieldCount
        || loadedFields[(u32)activeBuffer] != fieldNumber || scanline >= totalLines
        || previousCommand > 10 || !seekSpeed || seekSpeed > 64 || volume > 15 || brightness > 15 || osdMode > 2
        || (u32)titleState > (u32)TitleState::Stream || titleCycles > TitleCycles) {
        return fail("invalid MovieCart cursor state");
      }
      updateStatus();
    }
  }
};
