struct InputHotkey;

struct HotkeySettings : PanelItem {
  HotkeySettings(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh() -> void;
  auto update() -> void;
  auto eventAssign() -> void;
  auto eventAssignNext() -> void;
  auto eventClear() -> void;
  auto eventChange() -> void;
  auto eventInput(shared_pointer<HID::Device>, uint group, uint input, int16_t oldValue, int16_t newValue) -> void;

  Label headerLabel{this, Size{~0, 0}};
  TableView hotkeyList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Widget spacer{&controlLayout, Size{~0, 0}, 0};
    Button assignButton{&controlLayout, Size{80_sx, 0}};
    Button clearButton{&controlLayout, Size{80_sx, 0}};

  maybe<InputHotkey&> assigning;
  vector<TableViewItem> assigningQueue;
};
