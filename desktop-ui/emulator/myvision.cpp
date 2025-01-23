struct MyVision: Emulator {
  MyVision();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer < vfs::directory > override;
};

MyVision::MyVision() {
  manufacturer = "Nichibutsu";
  name = "MyVision";
  {
    InputPort port {
      "MyVision"
    }; {
      InputDevice device {
        "Controls"
      };
      device.digital("UP [B]", virtualPorts[0].pad.up);
      device.digital("DOWN [C]", virtualPorts[0].pad.down);
      device.digital("LEFT [A]", virtualPorts[0].pad.left);
      device.digital("RIGHT [D]", virtualPorts[0].pad.right);
      device.digital("ACTION [E]", virtualPorts[0].pad.south);

      device.digital("1", virtualPorts[0].pad.east);
      device.digital("2", virtualPorts[0].pad.west);
      device.digital("3", virtualPorts[0].pad.north);
      device.digital("4", virtualPorts[0].pad.l_bumper);
      device.digital("5", virtualPorts[0].pad.r_bumper);
      device.digital("6", virtualPorts[0].pad.l_trigger);
      device.digital("7", virtualPorts[0].pad.r_trigger);
      device.digital("8", virtualPorts[0].pad.lstick_click);
      device.digital("9", virtualPorts[0].pad.rstick_click);
      device.digital("10", virtualPorts[0].pad.lstick_up);
      device.digital("11", virtualPorts[0].pad.lstick_down);
      device.digital("12", virtualPorts[0].pad.lstick_left);
      device.digital("13", virtualPorts[0].pad.lstick_right);
      device.digital("14", virtualPorts[0].pad.rstick_up);
      port.append(device);
    }

    ports.append(port);
  }
}

auto MyVision::load() -> LoadResult {
  game = mia::Medium::create("MyVision");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("MyVision");
  result = system->load();
  if(result != successful) return result;

  if (!ares::MyVision::load(root, {
      "[Nichibutsu] MyVision"
  })) return otherError;

  if (auto port = root -> find < ares::Node::Port > ("Cartridge Slot")) {
    port -> allocate();
    port -> connect();
  }

  return successful;
}

auto MyVision::save() -> bool {
  root -> save();
  system -> save(system -> location);
  game -> save(game -> location);
  return true;
}

auto MyVision::pak(ares::Node::Object node) -> shared_pointer < vfs::directory > {
  if (node -> name() == "MyVision") return system -> pak;
  if (node -> name() == "MyVision Cartridge") return game -> pak;
  return {};
}
