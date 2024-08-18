struct GameBrowserEntry {
  string title;
  string name;
  string path;
};

struct GameBrowserWindow : Window {
  GameBrowserWindow();
  auto show(shared_pointer<Emulator> emulator) -> void;
  auto eventChange() -> void;

  HorizontalLayout layout{this};
  TableView gameList{&layout, Size{~0, ~0}};

  vector<GameBrowserEntry> games;
  shared_pointer<Emulator> emulator;
};

namespace Instances { extern Instance<GameBrowserWindow> gameBrowserWindow; }
extern GameBrowserWindow& gameBrowserWindow;
