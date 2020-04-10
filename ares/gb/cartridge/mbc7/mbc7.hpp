struct MBC7 : Mapper {
  Node::Axis x;
  Node::Axis y;

  enum : uint { Center = 0x81d0 };  //not 0x8000

  //mbc7.cpp
  auto load(Node::Object) -> void override;
  auto load(Markup::Node document) -> void override;
  auto save(Markup::Node document) -> void override;
  auto main() -> void override;
  auto read(uint16 address) -> uint8 override;
  auto write(uint16 address, uint8 data) -> void override;
  auto power() -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void override;

  struct EEPROM : M93LCx6 {
    //eeprom.cpp
    auto load(Markup::Node) -> void;
    auto save(Markup::Node) -> void;
    auto power() -> void;
    auto main() -> void;
    auto readIO() -> uint8;
    auto writeIO(uint8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    boolean select;  //CS
    boolean clock;   //CLK
  } eeprom;

  struct IO {
    struct ROM {
      uint8 bank = 0x01;
    } rom;
    struct RAM {
      uint1 enable[2];
    } ram;
    struct Accelerometer {
      uint16 x = Center;
      uint16 y = Center;
    } accelerometer;
  } io;
} mbc7;
