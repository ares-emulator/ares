struct GameImporter : Panel {
  GameImporter(View*);
  auto import(string system, const vector<string>& files) -> void;
  auto eventChange() -> void;
  auto eventClose() -> void;

  Label messageLabel{this, Size{~0, 0}};
  ListView importList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Widget controlSpacer{&controlLayout, Size{~0, 0}};
    Button abortButton{&controlLayout, Size{80_sx, 0}};
    Button closeButton{&controlLayout, Size{80_sx, 0}};

  bool processing = false;
};

extern GameImporter& gameImporter;
