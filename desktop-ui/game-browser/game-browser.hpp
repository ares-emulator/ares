struct GameBrowserEntry {
  string title;
  string name;
  string board;
  string path;
  bool available = false;
};

struct GameBrowserWindow : Window {
  GameBrowserWindow();
  auto show(std::shared_ptr<Emulator> emulator) -> void;
  auto refresh() -> void;

  VerticalLayout layout{this};
  HorizontalLayout searchLayout{&layout, Size{~0, 0}, 5};
    Label searchLabel{&searchLayout, Size{100, 0}};
    LineEdit searchInput{&searchLayout, Size{~0, 0}};
  HorizontalLayout browserLayout{&layout, Size{~0, ~0}, 5};
    ListView categoryList{&browserLayout, Size{150_sx, ~0}};
    TableView gameList{&browserLayout, Size{~0, ~0}};

  std::vector<GameBrowserEntry> games;
  std::shared_ptr<Emulator> emulator;
};

namespace Instances { extern Instance<GameBrowserWindow> gameBrowserWindow; }
extern GameBrowserWindow& gameBrowserWindow;
