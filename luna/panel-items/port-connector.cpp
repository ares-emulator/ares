PortConnector::PortConnector(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);

  renameAction.setIcon(Icon::Application::TextEditor).setText("Rename ...").onActivate([&] { eventRename(); });
  removeAction.setIcon(Icon::Action::Remove).setText("Delete ...").onActivate([&] { eventRemove(); });

  locationLabel.setMouseCursor(MouseCursor::Hand);
  locationLabel.setFont(Font().setBold());
  locationLabel.setForegroundColor({0, 0, 240});
  locationLabel.onMousePress([&](auto button) {
    if(button == Mouse::Button::Left) eventBrowse();
  });
  peripheralList.onActivate([&] { eventActivate(); });
  peripheralList.onChange([&] { eventChange(); });
  peripheralList.onContext([&] { eventContext(); });
  importButton.setText("Import ...").onActivate([&] { eventImport(); });
  acceptButton.setText("Create").onActivate([&] { eventActivate(); });
}

auto PortConnector::show() -> void {
  setVisible(true);
}

auto PortConnector::hide() -> void {
  setVisible(false);
  port = {};
}

auto PortConnector::isMiaType() const -> bool {
  if(!port) return false;  //should never occur
  if(port->type() == "Cartridge") return true;
  if(port->type() == "Floppy Disk") return true;
  if(port->type() == "Compact Disc") return true;
  return false;
}

auto PortConnector::refresh(ares::Node::Port port) -> void {
  this->port = port;
  importButton.setVisible(isMiaType());

  auto path = port->attribute("path");
  //is this the first time refreshing this port?
  if(!path) {
    //see if there's a shared mia path for it
    if(isMiaType()) {
      path = {Path::user(), "Emulation/", port->family(), "/"};
    }
    //if not, use a local path for it
    if(!directory::exists(path)) {
      path = {emulator.system.data, port->family(), " ", port->type(), "/"};
    }
    port->setAttribute("path", path);
  }

  //if this is a user path, replace the home directory with ~ for brevity
  if(path.beginsWith(Path::user())) {
    locationLabel.setText(string{path}.trimLeft(Path::user(), 1L).prepend("~/"));
  } else {
    locationLabel.setText(path);
  }

  peripheralList.reset();
  ListViewItem item{&peripheralList};
  item.setAttribute("type", "nothing");
  item.setIcon(Icon::Action::Remove).setText("Nothing");
  if(!port->connected()) item.setSelected().setFocused();

  if(string location = {emulator.system.templates, port->type(), "/"}) {
    for(auto& name : directory::folders(location)) {
    //if(!port->supported().find(name.trimRight("/", 1L))) continue;
      ListViewItem item{&peripheralList};
      item.setAttribute("type", "template");
      item.setAttribute("location", {location, name, "/"});
      item.setAttribute("path", location);
      item.setAttribute("name", name);
      item.setIcon(Icon::Action::Add).setText(name);
    }
  }

//for(auto& name : port->supported()) {
//  ListViewItem item{&peripheralList};
//  item.setAttribute("type", "template");
//  item.setAttribute("name", name);
//  item.setIcon(Icon::Action::Add).setText(name);
//}

  for(auto& name : directory::folders(path)) {
    ListViewItem item{&peripheralList};
    item.setAttribute("type", "peripheral");
    item.setAttribute("location", {path, name});
    item.setAttribute("path", path);
    item.setAttribute("name", name.trimRight("/", 1L));
    item.setIcon(Icon::Emblem::Folder).setText(name);
    //set a currently connected peripheral as the active item
    if(auto peripheral = port->connected()) {
      if(name == peripheral->attribute("name")) {
        item.setSelected().setFocused();
      }
    }
  }

  peripheralList.doChange();
}

auto PortConnector::eventImport() -> void {
  if(auto location = execute("mia", "--system", interface->name(), "--import").output) {
    refresh(port);
  }
}

auto PortConnector::eventActivate() -> void {
  if(auto item = peripheralList.selected()) {
    if(!port->hotSwappable() && emulator.system.power) {
      auto response = MessageDialog()
      .setText("The peripheral currently connected to this port isn't hot-swappable.\n"
               "Removing it anyway may crash the emulated system.\n"
               "What would you like to do?")
      .setTitle("Warning").setAlignment(program).question({"Force", "Power Off", "Cancel"});
      if(response == "Cancel") return;
      if(response == "Power Off") emulator.power(false);
    }

    auto name = item.attribute("name");

    if(item.attribute("type") == "nothing") {
      port->disconnect();
      program.refreshPanelList();
    }

    else if(item.attribute("type") == "template") {
      auto label = NameDialog()
      .setTitle({"Create New ", name})
      .setAlignment(program)
      .create(item.text());
      if(!label) return;

      auto source = item.attribute("location");
      auto target = string{emulator.system.data, port->family(), " ", port->type(), "/", label, "/"};
      if(directory::exists(target)) return (void)MessageDialog()
      .setText("A directory by this name already exists.")
      .setTitle("Error").setAlignment(program).error();

    //if(directory::create(target)) {
      if(directory::copy(source, target)) {
        if(auto peripheral = port->allocate(name)) {
          peripheral->setAttribute("location", target);
          peripheral->setAttribute("name", label);
          port->connect();
          inputManager.bind();  //bind any inputs this peripheral may contain
          program.refreshPanelList();
          refresh(port);
        }
      }
    }

    else if(item.attribute("type") == "peripheral") {
      auto location = item.attribute("location");
      auto connected = emulator.connected(location);

      //treat selecting the already-connected device as a no-op that aborts the port connection dialog
      if(connected && connected == port) return;

      if(connected) return (void)MessageDialog()
      .setText({"This peripheral is already connected to another port:\n\n", connected->name()})
      .setTitle("Error").setAlignment(program).error();

      if(auto markup = file::read({location, "settings.bml"})) {
        auto node = ares::Node::unserialize(markup);
        //update the location and name here, in case the folder moved since it was last connected
        node->setAttribute("location", location);
        node->setAttribute("name", item.attribute("name"));
        if(auto peripheral = port->allocate(node->name())) {
          peripheral->copy(node);
          port->connect();
          peripheral->copy(node);
        }
      } else {
        if(auto peripheral = port->allocate()) {
          peripheral->setAttribute("location", location);
          peripheral->setAttribute("name", item.attribute("name"));
          port->connect();
        }
      }
      inputManager.bind();  //bind any inputs this peripheral may contain
      program.refreshPanelList();
    }
  }
}

auto PortConnector::eventChange() -> void {
  acceptButton.setVisible(false);
  acceptButton.setEnabled(true);

  if(auto item = peripheralList.selected()) {
    if(item.attribute("type") == "nothing") {
      if(!port->connected()) {
        acceptButton.setText("Disconnect").setEnabled(false).setVisible();
      } else {
        acceptButton.setText("Disconnect").setVisible();
      }
    }

    if(item.attribute("type") == "template") {
      acceptButton.setText("Create").setVisible();
    }

    if(item.attribute("type") == "peripheral") {
      auto peripheral = port->connected();
      if(peripheral && peripheral->attribute("name") == item.text()) {
        acceptButton.setText("Connect").setEnabled(false).setVisible();
      } else {
        acceptButton.setText("Connect").setVisible();
      }
    }
  }

  controlLayout.resize();
}

auto PortConnector::eventContext() -> void {
  if(auto item = peripheralList.selected()) {
    if(item.attribute("type") == "peripheral") {
      contextMenu.setVisible();
    }
  }
}

auto PortConnector::eventBrowse() -> void {
  auto path = port->attribute("path");
  if(auto location = BrowserDialog()
  .setTitle({port->name(), " Location"})
  .setPath(path ? path : Path::user())
  .setAlignment(program).selectFolder()
  ) {
    port->setAttribute("path", location);
    refresh(port);
  }
}

auto PortConnector::eventRename() -> void {
  if(auto item = peripheralList.selected()) {
    if(item.attribute("type") == "peripheral") {
      auto location = item.attribute("location");
      auto path = item.attribute("path");
      auto name = item.attribute("name");
      if(auto rename = NameDialog()
      .setAlignment(program)
      .rename(name)
      ) {
        if(name == rename) return;

        if(directory::exists({path, rename})) return (void)MessageDialog()
        .setText("A directory by this name already exists.")
        .setTitle("Error").setAlignment(program).error();

        if(!directory::rename({path, name}, {path, rename})) return (void)MessageDialog()
        .setText("Failed to rename directory.")
        .setTitle("Error").setAlignment(program).error();

        //the location must be updated if this peripheral is already connected ...
        if(auto connector = emulator.connected(location)) {
          if(auto connected = connector->connected()) {
            connected->setAttribute("location", {path, rename, "/"});
            connected->setAttribute("name", rename);
            //the name will be updated in the system tree, so it must be refreshed:
            program.refreshPanelList();
          }
        }

        return refresh(port);
      }
    }
  }
}

auto PortConnector::eventRemove() -> void {
  if(auto item = peripheralList.selected()) {
    if(item.attribute("type") == "peripheral") {
      auto location = item.attribute("location");
      auto connected = emulator.connected(location);

      if(!port->hotSwappable() && emulator.system.power && emulator.connected(location)) return (void)MessageDialog()
      .setText({"This peripheral is not hot-swappable and is already connected to a port:\n\n", connected->name()})
      .setTitle("Error").setAlignment(program).error();

      if(MessageDialog()
      .setText("Are you really sure you want to delete this peripheral?\n"
               "All data will be permanently lost!")
      .setTitle("Warning").setAlignment(program).question() == "No"
      ) return;

      //must disconnect device before removing it, as disconnect will attempt to save data to disk
      if(connected) {
        port->disconnect();
        program.refreshPanelList();
      }

      if(!directory::remove(location)) return (void)MessageDialog()
      .setText("Failed to remove directory.")
      .setTitle("Error").setAlignment(program).error();

      refresh(port);
    }
  }
}
