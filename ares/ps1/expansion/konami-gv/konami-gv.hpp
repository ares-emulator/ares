struct KonamiGV : ExpansionDevice {
  KonamiGV(Node::Object parent);
  ~KonamiGV();
  auto type() const -> u32 override { return ExpansionPort::KonamiGVID; }
  auto save() -> void override;
  auto power(bool reset) -> void override;
  auto step(u32 clocks) -> void override;
  auto serialize(serializer&) -> void override;
  auto readByte(u32 address) -> u32 override;
  auto writeByte(u32 address, u32 data) -> void override;
  auto readHalf(u32 address) -> u32 override;
  auto writeHalf(u32 address, u32 data) -> void override;
  auto readWord(u32 address) -> u32 override;
  auto writeWord(u32 address, u32 data) -> void override;
  auto canReadDMA() -> bool override;
  auto canWriteDMA() -> bool override;
  auto readDMA() -> u32 override;
  auto writeDMA(u32 data) -> void override;

private:
  auto pollInputs() -> void;
  auto lightgunValue(u32 player, bool vertical) -> u32;
  auto readRegister(u32 offset) -> u32;
  auto writeRegister(u32 offset, u32 data) -> void;

  struct EEPROM : M93LCx6 {
    auto readPin() -> n1;
    auto writePins(n8 data) -> void;
    auto serialize(serializer&) -> void;
    boolean select;
    boolean clock;
  } eeprom;

  NCR53CF96 scsi;
  SCSICDROM cdrom;

  MBM29F016A fujitsu[4];
  LH28F400 sharp;

  auto flashRead(u32 offset, u32& value) -> bool;
  auto flashWrite(u32 offset, u32 value) -> bool;

  bool hasFujitsuFlash = false;
  bool hasSharpFlash = false;
  u32 flashAddress = 0;

  Node::Object port;
  Node::Object controls;
  Node::Input::Button up[4], down[4], left[4], right[4];
  Node::Input::Button button[4][3];
  Node::Input::Button start[4];
  Node::Input::Button coin[2];
  Node::Input::Button service;
  Node::Input::Button test;
  Node::Input::Axis axisX[2], axisY[2];
  Node::Input::Axis motionX[2], motionY[2];
  Node::Audio::Stream cdStream;
  Node::Video::Sprite crosshair[2];
  bool gunActive[2] = {true, false};
  s32 motionRemainderX[2] = {};
  s32 motionRemainderY[2] = {};
  s32 gunX[2] = {32768, 32768};
  s32 gunY[2] = {32768, 32768};
  auto updateCrosshair(u32 player) -> void;
  enum class Mode : u32 { None, Trackball2, Trackball1, Lightgun } mode = Mode::None;
  uPD4701A trackball[2];
  u32 frameAccumulator = 0;
  u32 audioAccumulator = 0;
  u32 audioSample = 0;
  s32 audioSectorLBA = -1;
  u8 audioSector[2352] = {};

  u32 p1 = 0xffff'ffff;
  u32 p2 = 0xffff'ffff;
  u32 p34 = 0xffff'ffff;
  u32 clockAccumulator = 0;
};
