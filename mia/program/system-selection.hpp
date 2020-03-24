struct SystemSelection : Panel {
  SystemSelection(View*);
  auto eventChange() -> void;

  ListView systemList{this, Size{~0, ~0}};
};

extern SystemSelection& systemSelection;
