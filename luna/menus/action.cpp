ActionMenu::ActionMenu(MenuBar* parent) : Menu(parent) {
  setText("System");
  create.setIcon(Icon::Action::Add).setText("Create").onActivate([&] {
    program.setPanelList(systemManager);
    systemManager.doCreate();
  });
  launch.setIcon(Icon::Media::Play).setText("Launch").setEnabled(false).onActivate([&] {
    systemManager.onActivate();
  });
  rename.setIcon(Icon::Application::TextEditor).setText("Rename").setEnabled(false).onActivate([&] {
    systemManager.doRename();
  });
  remove.setIcon(Icon::Action::Remove).setText("Delete").setEnabled(false).onActivate([&] {
    systemManager.doRemove();
  });
  quit.setIcon(Icon::Action::Quit).setText("Quit").onActivate([&] { emulator.quit(); });
}
