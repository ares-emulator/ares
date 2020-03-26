SystemCreation::SystemCreation(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  header.setText("Create New System").setFont(Font().setBold());
  systemList.onActivate([&] { eventAccept(); });
  systemList.onChange([&] { eventChange(); });
  nameLabel.setText("Name:");
  nameValue.onActivate([&] { eventAccept(); });
  createButton.setText("Create").onActivate([&] { eventAccept(); });
}

auto SystemCreation::show() -> void {
  refresh();
  setVisible(true);
}

auto SystemCreation::hide() -> void {
  setVisible(false);
}

auto SystemCreation::refresh() -> void {
  systemList.reset();
  for(auto& interface : interfaces) {
    ListViewItem item{&systemList};
    item.setAttribute<shared_pointer<ares::Interface>>("interface", interface);
    item.setText(interface->name());
  }
  systemList.doChange();
}

auto SystemCreation::eventChange() -> void {
  if(auto item = systemList.selected()) {
    if(auto interface = item.attribute<shared_pointer<ares::Interface>>("interface")) {
      nameValue.setText(interface->name());
      nameLabel.setEnabled(true);
      nameValue.setEnabled(true);
      createButton.setEnabled(true);
      return;
    }
  }

  //nothing selected
  nameLabel.setEnabled(false);
  nameValue.setEnabled(false).setText("");
  createButton.setEnabled(false);
}

auto SystemCreation::eventAccept() -> void {
  auto name = nameValue.text().strip();
  if(!name) return;
  name.append("/");

  auto location = Path::data;
  if(directory::exists({location, name})) {
    if(MessageDialog()
      .setTitle("Warning")
      .setText("A directory by this name already exists.\n"
               "Do you wish to delete the existing directory and create a new one?")
      .setAlignment(program).question() == "No"
    ) return;
    if(!directory::remove({location, name})) return (void)MessageDialog()
      .setTitle("Error")
      .setText("Failed to remove previous directory. The location may be read-only.")
      .setAlignment(program).error();
  }
  if(!directory::create({location, name})) return (void)MessageDialog()
    .setTitle("Error")
    .setText("Failed to create directory. Either the location is read-only, or the name contains invalid characters.")
    .setAlignment(program).error();

  if(auto interface = systemList.selected().attribute<shared_pointer<ares::Interface>>("interface")) {
    file::write({location, name, "manifest.bml"}, string{
      "system: ", interface->name(), "\n"
    });
  }

  systemManager.refresh();
  program.setPanelItem(home);
}
