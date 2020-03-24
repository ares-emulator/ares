struct InputMapper : PanelItem {
  InputMapper(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh(ares::Node::Object) -> void;
  auto update() -> void;

  auto eventAssignMouse(uint groupID, uint inputID) -> void;
  auto eventAssign() -> void;
  auto eventAssignNext() -> void;
  auto eventClear() -> void;
  auto eventChange() -> void;

  auto eventInput(shared_pointer<HID::Device>, uint group, uint input, int16_t oldValue, int16_t newValue, bool allowMouseInput = false) -> void;

  Label nameLabel{this, Size{~0, 0}};
  TableView inputList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Button mouseXaxis{&controlLayout, Size{100_sx, 0}};
    Button mouseYaxis{&controlLayout, Size{100_sx, 0}};
    Button mouseLeft{&controlLayout, Size{100_sx, 0}};
    Button mouseMiddle{&controlLayout, Size{100_sx, 0}};
    Button mouseRight{&controlLayout, Size{100_sx, 0}};
    Widget spacer{&controlLayout, Size{~0, 0}, 0};
    Button assignButton{&controlLayout, Size{80_sx, 0}};
    Button clearButton{&controlLayout, Size{80_sx, 0}};

  ares::Node::Object node;
  ares::Node::Input assigning;
  vector<TableViewItem> assigningQueue;
};
