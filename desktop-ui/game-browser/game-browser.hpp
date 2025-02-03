struct GameBrowserEntry {
  string title;
  string name;
  string board;
  string path;
};

struct GameBrowserWindow : Window {
  GameBrowserWindow();
  auto show(shared_pointer<Emulator> emulator) -> void;
  auto refresh() -> void;

  VerticalLayout layout{this};
  HorizontalLayout searchLayout{&layout, Size{~0, 0}, 5};
    Label searchLabel{&searchLayout, Size{100, 0}};
    LineEdit searchInput{&searchLayout, Size{~0, 0}};
  TableView gameList{&layout, Size{~0, ~0}};

  vector<GameBrowserEntry> games;
  shared_pointer<Emulator> emulator;
};

namespace Instances { extern Instance<GameBrowserWindow> gameBrowserWindow; }
extern GameBrowserWindow& gameBrowserWindow;
