auto CheatEditor::Cheat::update(string description, string code, bool enabled) -> Cheat& {
  this->description = description;
  this->code = code;
  this->enabled = enabled;

  //TODO: support other code formats based on the game system (e.g. GameShark, Game-Genie, etc.)

  addressValuePairs.reset();
  vector<string> codes = code.split("+");
  for(auto& code : codes) {
    auto parts = code.split(":");
    if(parts.size() != 2) continue;
    addressValuePairs.insert(string{"0x", parts[0]}.natural(), string{"0x",parts[1]}.natural());
  }

  return *this;
}

auto CheatEditor::construct(std::recursive_mutex *programMutexIn) -> void {
  this->programMutex = programMutexIn;
  setCollapsible();
  setVisible(false);
  cheatsLabel.setText("Cheats").setFont(Font().setBold());

  descriptionLabel.setText("Description:");
  codeLabel.setText("Code:");
  deleteButton.setText("Delete").onActivate([&] {
    lock_guard<recursive_mutex> lock(*programMutex);
    if(auto item = cheatList.selected()) {
      if(auto cheat = item.attribute<Cheat*>("cheat")) {
        if(auto c = cheats.find([cheat](auto& c) { return &c == cheat; })) {
          descriptionEdit.setText("");
          codeEdit.setText("");
          cheats.remove(*c);
          deleteButton.setEnabled(false);
          refresh();
        }
      }
    }
  }).setEnabled(false);

  saveButton.setText("Save").onActivate([&] {
    lock_guard<recursive_mutex> lock(*programMutex);
    string description = descriptionEdit.text();
    string code = codeEdit.text();

    if(auto c = cheats.find([description](auto& c) { return c.description == description; })) {
      cheats[*c].update(description, code);
    } else {
      cheats.append(Cheat().update(description, code));
    }

    refresh();
  });

  cheatList.onToggle([&](auto cell) {
    lock_guard<recursive_mutex> lock(*programMutex);
    if(auto item = cheatList.selected()) {
      if(auto cheat = item.attribute<Cheat*>("cheat")) {
        cheat->enabled = cell.checked();
      }
    }
  });

  cheatList.onChange([&] {
    lock_guard<recursive_mutex> lock(*programMutex);
    deleteButton.setEnabled(false);
    descriptionEdit.setText("");
    codeEdit.setText("");

    if(auto item = cheatList.selected()) {
      if(auto cheat = item.attribute<Cheat*>("cheat")) {
        descriptionEdit.setText(cheat->description);
        codeEdit.setText(cheat->code);
        deleteButton.setEnabled(true);
      }
    }
  });
}

auto CheatEditor::reload() -> void {
  lock_guard<recursive_mutex> lock(*programMutex);
  cheats.reset();

  location = emulator->locate(emulator->game->location, {".cheats.bml"});
  if(file::exists(location)) {
    auto document = BML::unserialize(string::read(location), " ");
    for(auto cheatNode : document.find("cheat")) {
      cheats.append(Cheat().update(
        cheatNode["description"].text(),
        cheatNode["code"].text(),
        cheatNode["enabled"].boolean()
      ));
    }
  }

  refresh();
}

auto CheatEditor::refresh() -> void {
  lock_guard<recursive_mutex> lock(*programMutex);
  cheatList.reset();
  cheatList.setHeadered();
  cheatList.append(TableViewColumn());
  cheatList.append(TableViewColumn().setText("Description").setExpandable());
  cheatList.append(TableViewColumn().setText("Code").setAlignment(1.0));

  for(auto& cheat : cheats) {
    TableViewItem item{&cheatList};
    item.setAttribute<Cheat*>("cheat", &cheat);
    item.append(TableViewCell().setCheckable().setChecked(cheat.enabled));
    item.append(TableViewCell().setText(cheat.description));
    item.append(TableViewCell().setText(cheat.code));
  }

  cheatList.resizeColumns();
  cheatList.column(0).setWidth(16);
}

auto CheatEditor::unload() -> void {
  lock_guard<recursive_mutex> lock(*programMutex);
  bool hasCheats = cheats.size() > 0;
  bool isCheatLocation = location.endsWith(".cheats.bml");

  if(hasCheats && isCheatLocation) {
    auto fp = file::open(location, file::mode::write);
    if(fp) {
      fp.print("cheats\n");
      fp.print("  revision: ", chrono::local::date(), "\n\n");
      for(auto& cheat : cheats) {
        fp.print("cheat\n");
        fp.print("  description: ", cheat.description, "\n");
        fp.print("  code: ", cheat.code, "\n");
        fp.print("  enabled: ", cheat.enabled, "\n");
        fp.print("\n");
      }
    }
  }

  if(!hasCheats && isCheatLocation && file::exists(location)) {
    file::remove(location);
  }

  location = "";
}

auto CheatEditor::find(uint address) -> maybe<u32> {
  lock_guard<recursive_mutex> lock(*programMutex);
  for(auto& cheat : cheats) {
    if(!cheat.enabled) continue;
    if(auto result = cheat.addressValuePairs.find(address)) return result();
  }
  return nothing;
}

auto CheatEditor::setVisible(bool visible) -> CheatEditor& {
  descriptionEdit.setText("");
  codeEdit.setText("");
  VerticalLayout::setVisible(visible);
  return *this;
}
