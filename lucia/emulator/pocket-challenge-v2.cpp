struct PocketChallengeV2 : Emulator {
  PocketChallengeV2();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

PocketChallengeV2::PocketChallengeV2() {
  medium = mia::medium("Pocket Challenge V2");
  manufacturer = "Benesse";
  name = "Pocket Challenge V2";
}

auto PocketChallengeV2::load(Menu menu) -> void {
  //the Pocket Challenge V2 game library is very small.
  //no titles for the system use portrait (vertical) orientation.
  //as such, neither the ares::WonderSwan core nor lucia provide an orientation setting.
}

auto PocketChallengeV2::load() -> bool {
  system.pak->append("bios.rom", {Resource::WonderSwan::Boot, sizeof Resource::WonderSwan::Boot});

  if(!ares::WonderSwan::load(root, "[Benesse] Pocket Challenge V2")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto PocketChallengeV2::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto PocketChallengeV2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Pocket Challenge V2") return system.pak;
  if(node->name() == "Pocket Challenge V2 Cartridge") return game.pak;
  return {};
}

auto PocketChallengeV2::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "Pass"  ) mapping = virtualPads[0].a;
  if(name == "Circle") mapping = virtualPads[0].b;
  if(name == "Clear" ) mapping = virtualPads[0].y;
  if(name == "View"  ) mapping = virtualPads[0].start;
  if(name == "Escape") mapping = virtualPads[0].select;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
