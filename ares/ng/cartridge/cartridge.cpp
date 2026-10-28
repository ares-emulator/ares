#include <ng/ng.hpp>

namespace ares::NeoGeo {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "board/board.cpp"
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Neo Geo Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");
  information.board = pak->attribute("board");

  if(Model::NeoGeoMVS()) system.dips.load(system.node, pak, {"DSW"}, 0, false, "arcade.bml");

  if(information.board == "rom_mslugx") board = std::make_unique<Board::MSlugX>(*this);
  if(information.board == "rom_fatfur2") board = std::make_unique<Board::FatFury2>(*this);
  if(information.board == "rom_kof98"  ) board = std::make_unique<Board::Kof98>(*this);

  if(information.board == "cmc42_bangbead") board = std::make_unique<Board::Cmc42>(*this, 0xf8);
  if(information.board == "cmc42_ganryu"  ) board = std::make_unique<Board::Cmc42>(*this, 0x07);
  if(information.board == "cmc42_kof99k"  ) board = std::make_unique<Board::Cmc42>(*this, 0x00);
  if(information.board == "cmc42_mslug3h" ) board = std::make_unique<Board::Cmc42>(*this, 0xad);
  if(information.board == "cmc42_nitd"    ) board = std::make_unique<Board::Cmc42>(*this, 0xff);
  if(information.board == "cmc42_preisle2") board = std::make_unique<Board::Cmc42>(*this, 0x9f);
  if(information.board == "cmc42_s1945p"  ) board = std::make_unique<Board::Cmc42>(*this, 0x05);
  if(information.board == "cmc42_sengoku3") board = std::make_unique<Board::Cmc42>(*this, 0xfe);
  if(information.board == "cmc42_zupapa"  ) board = std::make_unique<Board::Cmc42>(*this, 0xbd);

  if(information.board == "cmc50_jockeygp") board = std::make_unique<Board::JockeyGP>(*this);
  if(information.board == "cmc50_kof2001" ) board = std::make_unique<Board::Cmc50>(*this, 0x1e);
  if(information.board == "cmc50_kof2000n") board = std::make_unique<Board::Cmc50>(*this, 0x00, 2);

  if(information.board == "sma_kof99"  ) board = std::make_unique<Board::Sma<Board::Cmc42>>(*this, 0x00, Board::SmaTables::kof99);
  if(information.board == "sma_garou"  ) board = std::make_unique<Board::Sma<Board::Cmc42>>(*this, 0x06, Board::SmaTables::garou);
  if(information.board == "sma_garouh" ) board = std::make_unique<Board::Sma<Board::Cmc42>>(*this, 0x06, Board::SmaTables::garouh);
  if(information.board == "sma_mslug3" ) board = std::make_unique<Board::Sma<Board::Cmc42>>(*this, 0xad, Board::SmaTables::mslug3);
  if(information.board == "sma_mslug3a") board = std::make_unique<Board::Sma<Board::Cmc42>>(*this, 0xad, Board::SmaTables::mslug3a);
  if(information.board == "sma_kof2k"  ) board = std::make_unique<Board::Sma<Board::Cmc50>>(*this, 0x00, Board::SmaTables::kof2000);

  if(information.board == "pcm2_mslug4") board = std::make_unique<Board::Pcm2>(*this, 0x31,  8, 1);
  if(information.board == "pcm2_rotd"  ) board = std::make_unique<Board::Pcm2>(*this, 0x3f, 16, 1);
  if(information.board == "pcm2_pnyaa" ) board = std::make_unique<Board::Pcm2>(*this, 0x2e,  4, 1);

  if(information.board == "pvc_mslug5" ) board = std::make_unique<Board::Pvc>(*this, 0x19, Board::PvcTables::mslug5);
  if(information.board == "pvc_svc"    ) board = std::make_unique<Board::Pvc>(*this, 0x57, Board::PvcTables::svc);
  if(information.board == "pvc_kf2k3"  ) board = std::make_unique<Board::Pvc>(*this, 0x9d, Board::PvcTables::kof2003);
  if(information.board == "pvc_kf2k3h" ) board = std::make_unique<Board::Pvc>(*this, 0x9d, Board::PvcTables::kof2003h);

  if(information.board == "k2k2_kof2k2" ) board = std::make_unique<Board::K2k2>(*this, 0xec, Board::K2k2Tables::kof2002);
  if(information.board == "k2k2_matrim" ) board = std::make_unique<Board::K2k2>(*this, 0x6a, Board::K2k2Tables::matrim);
  if(information.board == "k2k2_samsh5" ) board = std::make_unique<Board::K2k2>(*this, 0x0f, Board::K2k2Tables::samsho5);
  if(information.board == "k2k2_sams5s" ) board = std::make_unique<Board::K2k2>(*this, 0x0d, Board::K2k2Tables::samsh5sp);

  if(!board) board = std::make_unique<Board::Rom>(*this);
  board->pak = pak;
  board->load();

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node || !board) return;
  if(Model::NeoGeoMVS()) system.dips.unload();
  board->unload();
  board->pak.reset();
  board.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  if(board) board->save();
}

auto Cartridge::power() -> void {
  if(board) board->power();
}

auto Cartridge::readP(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(board) return board->readP(upper, lower, address, data);
  return data;
}

auto Cartridge::writeP(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(board) return board->writeP(upper, lower, address, data);
}

auto Cartridge::readM(n32 address) -> n8 {
 if(board) return board->readM(address);
 return 0xff;
}

auto Cartridge::readC(n32 address) -> n8 {
  if(board) return board->readC(address);
  return 0xff;
}

auto Cartridge::readS(n32 address) -> n8 {
  if(board) return board->readS(address);
  return 0xff;
}

auto Cartridge::fixBankType() -> u32 {
  if(board) return board->fixBankType();
  return 0;
}

auto Cartridge::readVA(n32 address) -> n8 {
  if(board) return board->readVA(address);
  return 0xff;
}

auto Cartridge::readVB(n32 address) -> n8 {
  if(board) return board->readVB(address);
  return 0xff;
}

}
