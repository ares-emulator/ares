struct NodeManager : PanelList {
  NodeManager(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh() -> void;
  auto refresh(ares::Node::Object node, uint depth) -> void;
  auto refreshSettings() -> void;
  auto name(ares::Node::Object node, uint depth) -> string;
  auto onChange() -> void;

  ares::Node::Object root;
  ListView listView{this, Size{~0, ~0}};
};
