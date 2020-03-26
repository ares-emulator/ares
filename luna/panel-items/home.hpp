struct Home : PanelItem {
  Home(View*);
  auto show() -> void override;
  auto hide() -> void override;

  Frame frame{this, Size{~0, ~0}};
    Canvas iconCanvas{&frame};
};
