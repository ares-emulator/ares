ToolsMenu::ToolsMenu(MenuBar* parent) : Menu(parent) {
  setText("Tools");
  saveStateMenu.setText("Save State");
  saveState1.setText("Slot 1").onActivate([&] { emulator.saveState(1); });
  saveState2.setText("Slot 2").onActivate([&] { emulator.saveState(2); });
  saveState3.setText("Slot 3").onActivate([&] { emulator.saveState(3); });
  saveState4.setText("Slot 4").onActivate([&] { emulator.saveState(4); });
  saveState5.setText("Slot 5").onActivate([&] { emulator.saveState(5); });

  loadStateMenu.setText("Load State");
  loadState1.setText("Slot 1").onActivate([&] { emulator.loadState(1); });
  loadState2.setText("Slot 2").onActivate([&] { emulator.loadState(2); });
  loadState3.setText("Slot 3").onActivate([&] { emulator.loadState(3); });
  loadState4.setText("Slot 4").onActivate([&] { emulator.loadState(4); });
  loadState5.setText("Slot 5").onActivate([&] { emulator.loadState(5); });

  captureScreenshot.setIcon(Icon::Emblem::Image).setText("Capture Screenshot").onActivate([&] {
    emulator.requests.captureScreenshot = true;
  });

  pauseEmulation.setText("Pause Emulation");
}
