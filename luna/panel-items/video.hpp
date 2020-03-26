struct VideoSettings : PanelItem {
  VideoSettings(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh() -> void;
  auto eventChange() -> void;

  Frame frame{this, Size{~0, ~0}};
  VerticalLayout layout{&frame};

  Label driverHeader{&layout, Size{~0, 0}};
  HorizontalLayout driverLayout{&layout, Size{~0, 0}};
    Label driverLabel{&driverLayout, Size{0, 0}};
    ComboButton driverOption{&driverLayout, Size{0, 0}};
    Button changeButton{&driverLayout, Size{0, 0}};

  Label settingsHeader{&layout, Size{~0, 0}};
  VerticalLayout settingsLayout{&layout, Size{~0, 0}};
  HorizontalLayout optionsLayout{&settingsLayout, Size{~0, 0}};
    Label monitorLabel{&optionsLayout, Size{0, 0}};
    ComboButton monitorOption{&optionsLayout, Size{0, 0}};
    Label formatLabel{&optionsLayout, Size{0, 0}};
    ComboButton formatOption{&optionsLayout, Size{0, 0}};
  HorizontalLayout togglesLayout{&settingsLayout, Size{~0, 0}};
    CheckLabel exclusiveOption{&togglesLayout, Size{0, 0}};
    CheckLabel blockingOption{&togglesLayout, Size{0, 0}};
    CheckLabel flushOption{&togglesLayout, Size{0, 0}};

  Label colorHeader{&layout, Size{~0, 0}};
  TableLayout colorLayout{&layout, Size{~0, 0}};
    Label luminanceLabel{&colorLayout, Size{0, 0}};
    Label luminanceValue{&colorLayout, Size{50_sx, 0}};
    HorizontalSlider luminanceSlider{&colorLayout, Size{~0, 0}};
    Label saturationLabel{&colorLayout, Size{0, 0}};
    Label saturationValue{&colorLayout, Size{50_sx, 0}};
    HorizontalSlider saturationSlider{&colorLayout, Size{~0, 0}};
    Label gammaLabel{&colorLayout, Size{0, 0}};
    Label gammaValue{&colorLayout, Size{50_sx, 0}};
    HorizontalSlider gammaSlider{&colorLayout, Size{~0, 0}};
  Widget spacer{&layout, Size{~0, ~0}};
  Viewport viewport{&layout, Size{1, 1}};
};
