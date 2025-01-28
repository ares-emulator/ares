#include "../../ares/n64/nand/ecc.hpp"

struct iQuePlayer : Emulator {
  iQuePlayer();
  auto load() -> bool override;
  //auto load(Menu) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  auto loadSystem(shared_pointer<mia::Pak> pak, string name, string extension, string& path) -> bool;

  //shared_pointer<mia::Pak> disk;
};

iQuePlayer::iQuePlayer() {
  manufacturer = "Nintendo";
  name = "iQue Player";

  firmware.append({"BootROM", "China", "d3b78c1aca4070e8df8496f2aa894a2ef70def725eb47668e3485ce268b974df"});

  for(auto id : range(4)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.analog ("L-Up",    virtualPorts[id].pad.lstick_up);
    device.analog ("L-Down",  virtualPorts[id].pad.lstick_down);
    device.analog ("L-Left",  virtualPorts[id].pad.lstick_left);
    device.analog ("L-Right", virtualPorts[id].pad.lstick_right);
    device.digital("Up",      virtualPorts[id].pad.up);
    device.digital("Down",    virtualPorts[id].pad.down);
    device.digital("Left",    virtualPorts[id].pad.left);
    device.digital("Right",   virtualPorts[id].pad.right);
    device.digital("B",       virtualPorts[id].pad.west);
    device.digital("A",       virtualPorts[id].pad.south);
    device.digital("C-Up",    virtualPorts[id].pad.rstick_up);
    device.digital("C-Down",  virtualPorts[id].pad.rstick_down);
    device.digital("C-Left",  virtualPorts[id].pad.rstick_left);
    device.digital("C-Right", virtualPorts[id].pad.rstick_right);
    device.digital("L",       virtualPorts[id].pad.l_bumper);
    device.digital("R",       virtualPorts[id].pad.r_bumper);
    device.digital("Z",       virtualPorts[id].pad.r_trigger);
    device.digital("Start",   virtualPorts[id].pad.start);
    device.rumble ("Rumble",  virtualPorts[id].pad.rumble);
    device.analog ("X-Axis",  virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.analog ("Y-Axis",  virtualPorts[id].pad.lstick_up,   virtualPorts[id].pad.lstick_down);

    if(id == 0)
      device.digital("Power", virtualPorts[id].pad.select);
    port.append(device); }

  { InputDevice device{"Mouse"};
    device.relative("X",     virtualPorts[id].mouse.x);
    device.relative("Y",     virtualPorts[id].mouse.y);
    device.digital ("Left",  virtualPorts[id].mouse.left);
    device.digital ("Right", virtualPorts[id].mouse.right);
    port.append(device); }
  
    ports.append(port);
  }
}

auto iQuePlayer::loadSystem(shared_pointer<mia::Pak> pak, string name, string extension, string& path) -> bool {
  // this should be merged into emulator.cpp:Emulator::load
  string location;
  BrowserDialog dialog;
  dialog.setTitle({"Load iQue Player ", name});
  dialog.setPath(path ? path : Path::desktop());
  dialog.setAlignment(presentation);
  string filters{"*.zip:"};
  for(auto& extension : pak->extensions()) {
    filters.append("*.", extension, ":");
  }
  //support both uppercase and lowercase extensions
  filters.append(string{filters}.upcase());
  filters.trimRight(":", 1L);
  filters.prepend(pak->name(), "|");
  dialog.setFilters({filters, "All|*"});
  location = program.openFile(dialog);

  if(location) {
    path = Location::dir(location);
    
    return pak->load(name, extension, location);
  }

  return false;
}

auto errorWriteable(string name) -> void {
  MessageDialog().setText({
    "Error: ", name, " must be provided.\n",
  }).information();
}

auto iQuePlayer::load() -> bool {
  game = mia::Medium::create("iQue Player");
  if(!game->load("[system]")) return false;

  system = mia::System::create("iQue Player");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  string bb_dir = Path::desktop();
  
  while(true) {
    bool loaded = system->pak->read("nand.flash")->attribute("loaded") == "true";
    if(loaded) {
      auto size = system->pak->read("nand.flash")->size();

      if((size != 64_MiB) && (size != 128_MiB))
        loaded = false;
    }
    
    if(loaded)
      break;

    if(!loadSystem(system, "nand.flash", ".flash", bb_dir)) return errorWriteable("nand.flash"), false;
  }

  while(true) {
    bool loaded = system->pak->read("spare.flash")->attribute("loaded") == "true";
    if(loaded) {
      auto size = system->pak->read("spare.flash")->size();
      auto nand_size = system->pak->read("nand.flash")->size();

      if(((size / 16) != (nand_size / 16384)) && ((size / 16) != (nand_size / 512)))
        loaded = false;
    }

    if(loaded)
      break;

    if(!loadSystem(system, "spare.flash", ".flash", bb_dir)) return errorWriteable("spare.flash"), false;
  }

  auto spareSize = system->pak->read("spare.flash")->size();
  auto nandSize = system->pak->read("nand.flash")->size();

  if (spareSize * 0x4000 == nandSize * 0x10) {
    //We got block spares, convert to page spares
    if (auto fpSpare = system->pak->write("spare.flash")) {
      if (auto fpNand = system->pak->read("nand.flash")) {
        auto targetSize = 0x10 * nandSize / 0x200;
        //Read block spares
        vector<u8> blockSpare;
        blockSpare.reserve(spareSize);
        blockSpare.resize(spareSize);
        fpSpare->read({blockSpare.data(), blockSpare.size()});
        //Compute page spares
        vector<u8> pageSpare;
        pageSpare.reserve(targetSize);
        pageSpare.resize(targetSize);
        //For each block:
        for (auto blockNum : range(nandSize / 0x4000)) {
          printf("computing page spares for block %u\n", u32(blockNum));
          //For each page:
          for (auto i : range(32)) {
            auto pageNum = blockNum * 32 + i;
            //First 8 bytes are copied from the block spare verbatim
            memcpy(&pageSpare[pageNum * 0x10], &blockSpare[blockNum * 0x10], 8);
            //Last 8 bytes are the computed ecc over this page
            u8 page[0x200];
            fpNand->read(page);
            u8 ecc[8];
            ECC::computePageECC(page, &pageSpare[pageNum * 0x10 + 8]);
          }
        }
        //Resize file
        fpSpare->seek(0);
        fpSpare->resize(targetSize);
        //Write page spares
        fpSpare->write(pageSpare);
      }
    }
  }

  if(system->pak->read("virage2.flash")->attribute("loaded") != "true") {
    if(!loadSystem(system, "virage2.flash", ".flash", bb_dir)) return errorWriteable("virage2.flash"), false;
  }

  if(!system->save()) return false;

  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);
#if defined(VULKAN)
  ares::Nintendo64::option("Enable GPU acceleration", true);
#else
  ares::Nintendo64::option("Enable GPU acceleration", false);
#endif
  ares::Nintendo64::option("Disable Video Interface Processing", settings.video.disableVideoInterfaceProcessing);
  ares::Nintendo64::option("Weave Deinterlacing", settings.video.weaveDeinterlacing);
  ares::Nintendo64::option("Homebrew Mode", settings.general.homebrewMode);
  ares::Nintendo64::option("Recompiler", !settings.general.forceInterpreter);
  ares::Nintendo64::option("NAND64 0", settings.nintendo64.nand64[0]);
  ares::Nintendo64::option("NAND64 1", settings.nintendo64.nand64[1]);
  ares::Nintendo64::option("NAND64 2", settings.nintendo64.nand64[2]);
  ares::Nintendo64::option("NAND64 3", settings.nintendo64.nand64[3]);
  ares::Nintendo64::option("NAND128 0", settings.nintendo64.nand128[0]);
  ares::Nintendo64::option("NAND128 1", settings.nintendo64.nand128[1]);
  ares::Nintendo64::option("NAND128 2", settings.nintendo64.nand128[2]);
  ares::Nintendo64::option("NAND128 3", settings.nintendo64.nand128[3]);
  ares::Nintendo64::option("Is 128", system->pak->read("nand.flash")->size() == 128_MiB);

  if(!ares::Nintendo64::load(root, "[iQue] iQue Player")) return false;

  if(auto port = root->find<ares::Node::Port>("Card Slot")) {
    port->allocate();
    port->connect();
  }

  auto controllers = 4;
  for(auto id : range(controllers)) {
    if(auto port = root->find<ares::Node::Port>({"Controller Port ", 1 + id})) {
      auto peripheral = port->allocate("Gamepad");
      port->connect();
    }
  }

  program.showLED(true);
  return true;
}

/*auto iQuePlayer::load(Menu menu) -> void {

}*/

auto iQuePlayer::unload() -> void {
  Emulator::unload();
  program.showLED(false);

  gamepad.reset();
}

auto iQuePlayer::save() -> bool {
  root->save();
  system->save(system->location);
  return true;
}

auto iQuePlayer::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "iQue Player") return system->pak;
  if(node->name() == "iQue Card") return game->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}
