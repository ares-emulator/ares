GameManager::GameManager(View* parent) : Panel(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  pathLabel.setFont(Font().setBold()).setForegroundColor({0, 0, 240});
  pathLabel.onMouseRelease([&](auto button) {
    if(!path || button != Mouse::Button::Left) return;
    if(auto location = BrowserDialog()
    .setTitle({"Set ", system, " Games Location"})
    .setPath(path)
    .setAlignment(programWindow)
    .selectFolder()
    ) {
      path = location;
      pathLabel.setText(string{path}.replace(Path::user(), "~/"));
      refresh();
    }
  });
  importButton.setText("Import ...").onActivate([&] {
    auto pak = mia::Medium::create(system);
    auto extensions = pak->extensions();
    for(auto& extension : extensions) extension.prepend("*.");
    if(auto files = BrowserDialog()
    .setTitle({"Import ", system, " Games"})
    .setPath(settings.recent)
    .setFilters({{system, "|", extensions.merge(":"), ":*.zip:", extensions.merge(":").upcase(), ":*.ZIP"}, "All|*"})
    .setAlignment(programWindow)
    .openFiles()
    ) {
      settings.recent = Location::path(files.first());
      gameImporter.import(system, files);
    }
  });
}

auto GameManager::select(string system) -> void {
  path = {Path::user(), "Emulation/", system, "/"};
  pathLabel.setText(string{path}.replace(Path::user(), "~/"));
  this->system = system;
  refresh();
}

auto GameManager::refresh() -> void {
  gameList.reset();
  if(!path) return;

  for(auto& name : directory::folders(path)) {
    ListViewItem item{&gameList};
    item.setIcon(Icon::Emblem::Folder);
    item.setText(string{name}.trimRight("/", 1L));
  }

  programWindow.show(*this);
  Application::processEvents();
  gameList.resizeColumn();
}
