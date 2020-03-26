SystemMenu::SystemMenu(MenuBar* parent) : Menu(parent) {
  setText("System");
  power.setText("Power").onToggle([&] { emulator.power(power.checked()); });
  unload.setIcon(Icon::Go::Home).setText("Unload").onActivate([&] {
    emulator.unload();
    program.showPanels();
    program.setOverviewMode();
    program.setTitle({"luna v", ares::Version});
  });
}
