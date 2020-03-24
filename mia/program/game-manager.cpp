GameManager::GameManager(View* parent) : Panel(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  pathLabel.setFont(Font().setBold()).setForegroundColor({0, 0, 240});
  pathLabel.onMouseRelease([&](auto button) {
    if(!path || button != Mouse::Button::Left) return;
    if(auto location = BrowserDialog()
    .setTitle({"Set ", system, " Games Location"})
    .setPath(*path)
    .setAlignment(programWindow)
    .selectFolder()
    ) {
      *path = location;
      pathLabel.setText(*path);
      refresh();
    }
  });
  importButton.setText("Import ...").onActivate([&] {
    if(auto files = BrowserDialog()
    .setTitle({"Import ", system, " Games"})
    .setPath(settings.recent)
    .setAlignment(programWindow)
    .openFiles()
    ) {
      settings.recent = Location::path(files.first());
      gameImporter.import(system, files);
    }
  });
}

auto GameManager::select(string system) -> void {
  path.reset();
  for(auto& medium : media) {
    if(medium->name() != system) continue;
    path = medium->pathname;
  }
  if(!path) return;

  pathLabel.setText(*path);
  this->system = system;
  refresh();
}

auto GameManager::refresh() -> void {
  gameList.reset();
  if(!path) return;

  for(auto& name : directory::folders(*path)) {
    ListViewItem item{&gameList};
    item.setIcon(Icon::Emblem::Folder);
    item.setText(string{name}.trimRight("/", 1L));
  }

  programWindow.show(*this);
  gameList.resizeColumn();
}
