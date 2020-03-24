struct GameManager : Panel {
  GameManager(View*);
  auto select(string system) -> void;
  auto refresh() -> void;

  Label pathLabel{this, Size{~0, 0}};
  ListView gameList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Widget controlSpacer{&controlLayout, Size{~0, 0}};
    Button importButton{&controlLayout, Size{80_sx, 0}};

  string system;
  maybe<string&> path;
};

extern GameManager& gameManager;
