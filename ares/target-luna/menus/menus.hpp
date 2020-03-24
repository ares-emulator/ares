struct ActionMenu : Menu {
  ActionMenu(MenuBar*);
  MenuItem create{this};
  MenuItem launch{this};
  MenuItem rename{this};
  MenuItem remove{this};
  MenuSeparator quitSeparator{this};
  MenuItem quit{this};
};

struct SystemMenu : Menu {
  SystemMenu(MenuBar*);
  MenuCheckItem power{this};
  MenuSeparator unloadSeparator{this};
  MenuItem unload{this};
};

struct SettingsMenu : Menu {
  auto updateShaders() -> void;

  SettingsMenu(MenuBar*);
  Menu outputMenu{this};
    MenuRadioItem outputCenter{&outputMenu};
    MenuRadioItem outputScale{&outputMenu};
    MenuRadioItem outputStretch{&outputMenu};
    Group videoOutputGroup{&outputCenter, &outputScale, &outputStretch};
    MenuSeparator outputSeparator{&outputMenu};
    MenuCheckItem aspectCorrection{&outputMenu};
  Menu scaleMenu{this};
    MenuItem scale2x{&scaleMenu};
    MenuItem scale3x{&scaleMenu};
    MenuItem scale4x{&scaleMenu};
  Menu shaderMenu{this};
  MenuSeparator separator{this};
  MenuCheckItem muteAudio{this};
  MenuCheckItem showStatusBar{this};
  MenuCheckItem showSystemPanels{this};
};

struct ToolsMenu : Menu {
  ToolsMenu(MenuBar*);
  Menu saveStateMenu{this};
    MenuItem saveState1{&saveStateMenu};
    MenuItem saveState2{&saveStateMenu};
    MenuItem saveState3{&saveStateMenu};
    MenuItem saveState4{&saveStateMenu};
    MenuItem saveState5{&saveStateMenu};
  Menu loadStateMenu{this};
    MenuItem loadState1{&loadStateMenu};
    MenuItem loadState2{&loadStateMenu};
    MenuItem loadState3{&loadStateMenu};
    MenuItem loadState4{&loadStateMenu};
    MenuItem loadState5{&loadStateMenu};
  MenuSeparator stateSeparator{this};
  MenuItem captureScreenshot{this};
  MenuCheckItem pauseEmulation{this};
};

struct HelpMenu : Menu {
  HelpMenu(MenuBar*);
  MenuItem about{this};
};
