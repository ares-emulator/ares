struct Home : Panel {
  Home(View*);

  Frame frame{this, Size{~0, ~0}};
    Canvas iconCanvas{&frame};
};

extern Home& home;
