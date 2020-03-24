struct SystemOverview : PanelItem {
  SystemOverview(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh() -> void override;
  auto scan(ares::Node::Object node, uint depth = 0) -> void;

  Label header{this, Size{~0, 0}};
  ListView nodeList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Label nameLabel{&controlLayout, Size{~0, 0}};
    Button removeButton{&controlLayout, Size{80, 0}};
    Button renameButton{&controlLayout, Size{80, 0}};
    Button launchButton{&controlLayout, Size{80, 0}};
};
