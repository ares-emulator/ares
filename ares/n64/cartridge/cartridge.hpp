struct Cartridge;
#include "board/board.hpp"

struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  auto title() const -> string { return board->title(); }
  auto region() const -> string { return information.region; }
  auto cic() const -> string { return board->cic(); }

  auto tickRTC() -> void { board->tickRTC(); }
  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 { return board->joybusComm(send, recv, input, output); }


  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto loadBoard(string) -> Markup::Node;
  auto connect() -> void;
  auto disconnect() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;
  auto readHalf(u32 address) -> u16;
  auto writeHalf(u32 address, u16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  unique_pointer<Board::Interface> board;

  struct Information {
    string title;
    string region;
    string board;
    string cic;
  } information;

  friend struct Board::Interface;
};

#include "slot.hpp"
extern Cartridge& cartridge;
