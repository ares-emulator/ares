namespace ares::ColecoVision {
  auto load(Node::System& node, string name) -> bool;
}

struct ColecoVision : Emulator {
  ColecoVision();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

ColecoVision::ColecoVision() {
  medium = mia::medium("ColecoVision");
  manufacturer = "Coleco";
  name = "ColecoVision";

  firmware.append({"BIOS", "World"});
}

auto ColecoVision::load() -> bool {
  auto region = Emulator::region();
  if(!ares::ColecoVision::load(root, {"[Coleco] ColecoVision (", region, ")"})) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto ColecoVision::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "bios.rom") {
    return loadFirmware(firmware[0]);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  return {};
}

auto ColecoVision::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "L"    ) mapping = virtualPads[0].select;
  if(name == "R"    ) mapping = virtualPads[0].start;
  if(name == "1"    ) mapping = virtualPads[0].a;
  if(name == "2"    ) mapping = virtualPads[0].b;
  if(name == "3"    ) mapping = virtualPads[0].c;
  if(name == "4"    ) mapping = virtualPads[0].x;
  if(name == "5"    ) mapping = virtualPads[0].y;
  if(name == "6"    ) mapping = virtualPads[0].z;
  if(name == "7"    ) mapping = virtualPads[0].l1;
  if(name == "8"    ) mapping = virtualPads[0].r1;
  if(name == "9"    ) mapping = virtualPads[0].l2;
  if(name == "*"    ) mapping = virtualPads[0].r2;
  if(name == "0"    ) mapping = virtualPads[0].lt;
  if(name == "#"    ) mapping = virtualPads[0].rt;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
