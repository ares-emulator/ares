struct AudioSettings : PanelItem {
  AudioSettings(View*);
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
  HorizontalLayout deviceLayout{&settingsLayout, Size{~0, 0}};
    Label deviceLabel{&deviceLayout, Size{0, 0}};
    ComboButton deviceOption{&deviceLayout, Size{~0, 0}};
    Label frequencyLabel{&deviceLayout, Size{0, 0}};
    ComboButton frequencyOption{&deviceLayout, Size{0, 0}};
    Label latencyLabel{&deviceLayout, Size{0, 0}};
    ComboButton latencyOption{&deviceLayout, Size{0, 0}};
  HorizontalLayout togglesLayout{&settingsLayout, Size{~0, 0}};
    CheckLabel exclusiveOption{&togglesLayout, Size{0, 0}};
    CheckLabel blockingOption{&togglesLayout, Size{0, 0}};
    CheckLabel dynamicOption{&togglesLayout, Size{0, 0}};

  Label effectsHeader{&layout, Size{~0, 0}};
  TableLayout effectsLayout{&layout, Size{~0, 0}};
    Label skewLabel{&effectsLayout, Size{0, 0}};
    Label skewValue{&effectsLayout, Size{50_sx, 0}};
    HorizontalSlider skewSlider{&effectsLayout, Size{~0, 0}};
    Label volumeLabel{&effectsLayout, Size{0, 0}};
    Label volumeValue{&effectsLayout, Size{50_sx, 0}};
    HorizontalSlider volumeSlider{&effectsLayout, Size{~0, 0}};
    Label balanceLabel{&effectsLayout, Size{0, 0}};
    Label balanceValue{&effectsLayout, Size{50_sx, 0}};
    HorizontalSlider balanceSlider{&effectsLayout, Size{~0, 0}};
};
