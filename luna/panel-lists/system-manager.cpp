SystemManager::SystemManager(View* view) : PanelList(view, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  listView.onActivate([&] { onActivate(); });
  listView.onChange([&] { onChange(); });
  listView.onContext([&] { onContext(); });
  listCreate.setText("Create").setIcon(Icon::Action::Add).onActivate([&] { doCreate(); });
  listLaunch.setText("Launch").setIcon(Icon::Media::Play).onActivate([&] { onActivate(); });
  listRename.setText("Rename").setIcon(Icon::Application::TextEditor).onActivate([&] { doRename(); });
  listRemove.setText("Remove").setIcon(Icon::Action::Remove).onActivate([&] { doRemove(); });
}

auto SystemManager::show() -> void {
  listView.selectNone().doChange();
  refresh();
  setVisible(true);
}

auto SystemManager::hide() -> void {
  setVisible(false);
  actionMenu.rename.setEnabled(false);
  actionMenu.remove.setEnabled(false);
}

auto SystemManager::refresh() -> void {
  listView.reset();
  listView.doChange();

  ListViewItem item{&listView};
  item.setIcon(Icon::Action::Add);
  item.setText("Create New System");

  auto location = Path::data;
  for(auto& name : directory::folders(location)) {
    auto document = BML::unserialize(file::read({location, name, "/", "manifest.bml"}));
    if(!document) continue;

    auto system = document["system"].text();
    ListViewItem item{&listView};
    item.setAttribute("location", {location, name});
    item.setAttribute("path", location);
    item.setAttribute("name", name.trimRight("/", 1L));
    item.setAttribute("system", system);
    item.setIcon(Icon::Place::Server);
    item.setText(name);

    //hignlight any systems found that have not been compiled into this binary
    if(!interfaces.find([&](auto interface) { return interface->name() == system; })) {
      item.setForegroundColor({240, 80, 80});
      //only show the missing systems in advanced mode
      if(!settings.interface.advancedMode) item.remove();
    }
  }
}

auto SystemManager::onActivate() -> void {
  if(auto item = listView.selected()) {
    if(!item.attribute("path")) return;
    if(auto system = item.attribute("system")) {
      if(auto index = interfaces.find([&](auto interface) { return interface->name() == system; })) {
        emulator.create(interfaces[*index], {item.attribute("path"), item.attribute("name"), "/"});
      }
    }
  }
}

auto SystemManager::onChange() -> void {
  auto item = listView.selected();
  if(item && !item.attribute("location")) {
    program.setPanelItem(systemCreation);
    actionMenu.create.setEnabled(true);
    actionMenu.launch.setEnabled(false);
    actionMenu.rename.setEnabled(false);
    actionMenu.remove.setEnabled(false);
  } else if(item && item.attribute("location")) {
    systemOverview.refresh();
    program.setPanelItem(systemOverview);
    actionMenu.create.setEnabled(false);
    actionMenu.launch.setEnabled(true);
    actionMenu.rename.setEnabled(true);
    actionMenu.remove.setEnabled(true);
  } else {
    program.setPanelItem(home);
    actionMenu.create.setEnabled(true);
    actionMenu.launch.setEnabled(false);
    actionMenu.rename.setEnabled(false);
    actionMenu.remove.setEnabled(false);
  }
}

auto SystemManager::onContext() -> void {
  if(auto item = listView.selected()) {
    if(!item.attribute("path")) return;  //no context menu for "Create New System" option
    listCreate.setVisible(false);
    listLaunch.setVisible(true);
    listRename.setVisible(true);
    listRemove.setVisible(true);
    listMenu.setVisible();
  } else {
    listCreate.setVisible(true);
    listLaunch.setVisible(false);
    listRename.setVisible(false);
    listRemove.setVisible(false);
    listMenu.setVisible();
  }
}

auto SystemManager::doCreate() -> void {
  //clear the current selection to indicate that creation isn't related to the currently selected system item
  listView.selectNone();
  program.setPanelItem(systemCreation);
}

auto SystemManager::doRename() -> void {
  if(auto item = listView.selected()) {
    if(!item.attribute("path")) return;
    auto path = item.attribute("path");
    auto from = item.attribute("name");
    if(auto name = NameDialog()
    .setAlignment(program)
    .rename(from)
    ) {
      if(name == from) return;
      if(directory::exists({path, name})) return (void)MessageDialog()
      .setTitle("Error")
      .setText("A directory with this name already exists.\n"
               "Please choose a unique name.")
      .setAlignment(program)
      .error();
      if(!directory::rename({path, from}, {path, name})) return (void)MessageDialog()
      .setTitle("Error")
      .setText("Failed to rename directory.")
      .setAlignment(program)
      .error();
      refresh();
    }
  }
}

auto SystemManager::doRemove() -> void {
  if(auto item = listView.selected()) {
    if(!item.attribute("path")) return;
    if(MessageDialog()
    .setTitle({"Delete ", item.attribute("name")})
    .setText("Are you sure you want to permanently delete the selected system?\n"
             "All of its contents will be permanently lost!")
    .setAlignment(program)
    .question() != "Yes") return;
    auto location = string{item.attribute("path"), item.attribute("name"), "/"};
    if(!directory::remove(location)) return (void)MessageDialog()
    .setTitle("Error")
    .setText("Failed to delete directory.")
    .setAlignment(program)
    .error();
    program.setPanelItem(home);  //hide the system overview for the now-deleted system
    refresh();
  }
}
