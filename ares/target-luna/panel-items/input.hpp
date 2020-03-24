struct InputSettings : PanelItem {
  InputSettings(View*);
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

  Label focusLossHeader{&layout, Size{~0, 0}};
  HorizontalLayout focusLossLayout{&layout, Size{~0, 0}};
    RadioLabel focusPause{&focusLossLayout, Size{0, 0}};
    RadioLabel focusBlock{&focusLossLayout, Size{0, 0}};
    RadioLabel focusAllow{&focusLossLayout, Size{0, 0}};
    Group focusGroup{&focusPause, &focusBlock, &focusAllow};
};
