struct SystemManager : PanelList {
  SystemManager(View*);
  auto show() -> void override;
  auto hide() -> void override;
  auto refresh() -> void;

  auto onActivate() -> void;
  auto onChange() -> void;
  auto onContext() -> void;

  auto doCreate() -> void;
  auto doRename() -> void;
  auto doRemove() -> void;

  ListView listView{this, Size{~0, ~0}};
    PopupMenu listMenu;
      MenuItem listCreate{&listMenu};
      MenuItem listLaunch{&listMenu};
      MenuItem listRename{&listMenu};
      MenuItem listRemove{&listMenu};
};
