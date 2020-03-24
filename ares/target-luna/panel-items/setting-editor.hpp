struct SettingEditor : PanelItem {
  SettingEditor(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh(ares::Node::Setting setting = {}) -> void;

  auto eventChange() -> void;

  VerticalLayout layout{this, Size{~0, ~0}};
    Label nameLabel{&layout, Size{~0, 0}};
    HorizontalLayout latchedLayout{&layout, Size{~0, 0}};
      Label latchedLabel{&latchedLayout, Size{0, 0}};
      Label latchedValue{&latchedLayout, Size{~0, 0}};
    ListView valueList{&layout, Size{~0, ~0}};

  ares::Node::Setting setting;
};
