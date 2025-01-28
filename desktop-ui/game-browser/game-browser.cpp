#include "../desktop-ui.hpp"

namespace Instances { Instance<GameBrowserWindow> gameBrowserWindow; }
GameBrowserWindow& gameBrowserWindow = Instances::gameBrowserWindow();

GameBrowserWindow::GameBrowserWindow() {
  setDismissable();
  setSize({700_sx, 405_sy});
  setAlignment({1.0, 1.0});
  setMinimumSize({480_sx, 320_sy});
  gameList.setHeadered();

  gameList.onActivate([&](auto cell) {
    auto game = games[gameList.selected().offset()];
    if(program.load(emulator, game.path)) {
      setVisible(false);
    }
  });
}

auto GameBrowserWindow::show(shared_pointer<Emulator> emulator) -> void {
  this->emulator = emulator;
  games.reset();

  auto tmp = (shared_pointer<mia::Medium>)mia::Medium::create(emulator->medium);
  if(!tmp) {
    string text = {"Failed to load Medium: ", emulator->medium};
    MessageDialog().setTitle("Error").setText(text).setAlignment(presentation).error();
    return;
  }

  auto db = tmp->database();
  for(auto node : db.list) {
    if(node["type"].string() != "game") continue;
    auto path = settings.paths.arcadeRoms;
    if(!path) path = {mia::homeLocation(), "Arcade"};

    path = {path, "/", node["name"].string(), ".zip"};

    if(inode::exists(path)) {
      games.append({node["title"].string(), node["name"].string(), node["board"].string(), path});
    }
  }

  games.sort([](auto x, auto y) {
    return string::icompare(x.title, y.title) < 0;
  });

  setVisible();
  setFocused();
  setTitle({"Select ", emulator->medium, " Game"});
  gameList.reset();

  gameList.append(TableViewColumn().setText("Game Title").setExpandable());
  gameList.append(TableViewColumn().setText("Board").setExpandable());
  gameList.append(TableViewColumn().setText("MAME Name"));

  for(auto& game : games) {
    TableViewItem item{&gameList};
    item.append(TableViewCell().setText(game.title));
    item.append(TableViewCell().setText(game.board));
    item.append(TableViewCell().setText(game.name));
  }

  gameList.resizeColumns();
  gameList.setFocused();
}
