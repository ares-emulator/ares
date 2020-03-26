struct SettingsManager : PanelList {
  SettingsManager(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh() -> void;

  auto onChange() -> void;

  ListView listView{this, Size{~0, ~0}};
};
