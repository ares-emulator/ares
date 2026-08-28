#include <a26/a26.hpp>

namespace ares::Atari2600 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");
  information.board  = pak->attribute("board");

  if(information.board == "Linear")         board = std::make_unique<Board::Linear>(*this);
  if(information.board == "Activision8k")   board = std::make_unique<Board::Activision8k>(*this);
  if(information.board == "Atari8k")        board = std::make_unique<Board::Atari8k>(*this);
  if(information.board == "Atari8kSC")      board = std::make_unique<Board::Atari8k>(*this, true);
  if(information.board == "Atari16k")       board = std::make_unique<Board::Atari16k>(*this);
  if(information.board == "Atari16kSC")     board = std::make_unique<Board::Atari16k>(*this, true);
  if(information.board == "Atari32k")       board = std::make_unique<Board::Atari32k>(*this);
  if(information.board == "Atari32kSC")     board = std::make_unique<Board::Atari32k>(*this, true);
  if(information.board == "Commavid")       board = std::make_unique<Board::Commavid>(*this);
  if(information.board == "ParkerBros8k")   board = std::make_unique<Board::ParkerBros>(*this);
  if(information.board == "ParkerBros03E0") board = std::make_unique<Board::ParkerBros03E0>(*this);
  if(information.board == "JVP")            board = std::make_unique<Board::JVP>(*this);
  if(information.board == "Tigervision")    board = std::make_unique<Board::Tigervision>(*this);
  if(information.board == "UA8k")           board = std::make_unique<Board::UA8k>(*this);
  if(information.board == "UASW")           board = std::make_unique<Board::UA8k>(*this, true);
  if(information.board == "CbsRamPlus")     board = std::make_unique<Board::CbsRamPlus>(*this);
  if(information.board == "MNetwork")       board = std::make_unique<Board::MNetwork>(*this);
  if(information.board == "AmigaFC")        board = std::make_unique<Board::AmigaFC>(*this);
  if(information.board == "Wickstead")      board = std::make_unique<Board::Wickstead>(*this);
  if(information.board == "Jane")           board = std::make_unique<Board::Jane>(*this);
  if(information.board == "DPC")            board = std::make_unique<Board::DPC>(*this);
  if(information.board == "MegaBoy")        board = std::make_unique<Board::MegaBoy>(*this);
  if(information.board == "Atari32In1")     board = std::make_unique<Board::Atari32In1>(*this);

  //Homebrew
  if(information.board == "DPC+")           board = std::make_unique<Board::DPCPlus>(*this);
  if(information.board == "CDF")            board = std::make_unique<Board::CDF>(*this);
  if(information.board == "BUS")            board = std::make_unique<Board::BUS>(*this);
  if(information.board == "Chetiry")        board = std::make_unique<Board::Chetiry>(*this);
  if(information.board == "FA2")            board = std::make_unique<Board::FA2>(*this);
  if(information.board == "4KSC")           board = std::make_unique<Board::CPUWiz4KSC>(*this);
  if(information.board == "3E")             board = std::make_unique<Board::ThreeE>(*this);
  if(information.board == "3EX")            board = std::make_unique<Board::ThreeEX>(*this);
  if(information.board == "3E+")            board = std::make_unique<Board::ThreeEPlus>(*this);
  if(information.board == "Enhanced3F")     board = std::make_unique<Board::Enhanced3F>(*this);
  if(information.board == "4A50")           board = std::make_unique<Board::FourA50>(*this);
  if(information.board == "EF")             board = std::make_unique<Board::EF>(*this);
  if(information.board == "EFF")            board = std::make_unique<Board::EFF>(*this);
  if(information.board == "DF")             board = std::make_unique<Board::DF>(*this);
  if(information.board == "BF")             board = std::make_unique<Board::BF>(*this);
  if(information.board == "EFSC")           board = std::make_unique<Board::EFSC>(*this);
  if(information.board == "DFSC")           board = std::make_unique<Board::DFSC>(*this);
  if(information.board == "BFSC")           board = std::make_unique<Board::BFSC>(*this);
  if(information.board == "MDM")            board = std::make_unique<Board::MDM>(*this);
  if(information.board == "X07")            board = std::make_unique<Board::X07>(*this);
  if(information.board == "EconoBanking")   board = std::make_unique<Board::EconoBanking>(*this);
  if(information.board == "Superbanking")   board = std::make_unique<Board::Superbanking>(*this);

  if(!board) board = std::make_unique<Board::Interface>(*this);
  board->pak = pak;
  board->load();
  power(false);
}

auto Cartridge::disconnect() -> void {
  if(!node || !board) return;
  board->unload();
  board->pak.reset();
  board.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  if(board) board->save();
}

auto Cartridge::power(bool reset) -> void {
  if(board) board->power(reset);
}

auto Cartridge::read(n16 address, n8 data) -> n8 {
  if(!node) return data;
  if(board) return board->read(address, data);
  return data;
}

auto Cartridge::write(n16 address, n8 data) -> n8 {
  if(!node) return data;
  if(board) return board->write(address, data);
  return data;
}

auto Cartridge::armInvocation() const -> Harmony::Invocation {
  if(board) return board->armInvocation();
  return {};
}

auto Cartridge::readARM(u32 mode, n32 address, n32& data) -> Harmony::Access {
  if(board) return board->readARM(mode, address, data);
  return Harmony::Access::Unmapped;
}

auto Cartridge::writeARM(u32 mode, n32 address, n32 data) -> Harmony::Access {
  if(board) return board->writeARM(mode, address, data);
  return Harmony::Access::Unmapped;
}

auto Cartridge::trapARM(u32 address, n32& value, n32 argument) -> bool {
  if(board) return board->trapARM(address, value, argument);
  return false;
}

auto Cartridge::stepARM(u32 clocks) -> void {
  if(board) board->stepARM(clocks);
}

}
