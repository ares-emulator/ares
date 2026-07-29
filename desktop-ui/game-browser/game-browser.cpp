#include "../desktop-ui.hpp"

namespace Instances { Instance<GameBrowserWindow> gameBrowserWindow; }
GameBrowserWindow& gameBrowserWindow = Instances::gameBrowserWindow();

GameBrowserWindow::GameBrowserWindow() {
  setDismissable();
  setSize({700_sx, 405_sy});
  setAlignment({1.0, 1.0});
  setMinimumSize({480_sx, 320_sy});
  searchLayout.setPadding(5, 0);
  searchLabel.setText("Search:");
  categoryList->setUsesSidebarStyle();
  gameList.setHeadered();

  gameList.onActivate([&](auto cell) {
    auto name = cell.parent().attribute("name");
    for(auto& game : games) {
      if(game.name != name) continue;
      if(!game.available) {
        program.error({"ROM archive not found: ", game.path});
        return;
      }
      if(program.load(emulator, game.path)) setVisible(false);
      return;
    }
  });

  categoryList.onChange([&] {
    refresh();
  });

  searchInput.onChange([&] {
    refresh();
  });
}

auto GameBrowserWindow::show(std::shared_ptr<Emulator> emulator) -> void {
  this->emulator = emulator;
  games.clear();
  categoryList.reset();
  searchInput.setText();

  auto tmp = std::dynamic_pointer_cast<mia::Medium>(mia::Medium::create(emulator->medium));
  if(!tmp) {
    string text = {"Failed to load Medium: ", emulator->medium};
    program.error(text);
    return;
  }

  auto categories = emulator->gameBrowserCategories();
  ListViewItem all{&categoryList};
  all.setText("All");
  all.setAttribute("board", "");
  for(auto& category : categories) {
    ListViewItem item{&categoryList};
    item.setText(category.name);
    item.setAttribute("board", category.board);
  }

  auto romRoot = settings.paths.arcadeRoms;
  if(!romRoot) romRoot = {mia::homeLocation(), "Arcade"};

  auto db = tmp->database();
  for(auto node : db.list) {
    if(node["type"].string().size() && node["type"].string() != "game") continue;

    auto board = node["board"].string();
    auto supported = std::ranges::find_if(categories, [&](const auto& category) {
      return category.board == board;
    });
    if(supported == categories.end()) continue;

    auto path = string{romRoot, "/", node["name"].string(), ".zip"};
    games.push_back({node["title"].string(), node["name"].string(), board, path, inode::exists(path)});
  }

  std::ranges::sort(games, [](const auto& x, const auto& y) {
    return string::icompare(x.title, y.title) < 0;
  });

  all.setSelected();
  categoryList.resizeColumn();

  setVisible();
  setFocused();
  setTitle({"Select ", emulator->medium, " Game"});

  refresh();

  gameList.setFocused();
}

auto GameBrowserWindow::refresh() -> void {
  gameList.reset();

  gameList.append(TableViewColumn().setText("Game Title").setExpandable());
  gameList.append(TableViewColumn().setText("Board").setExpandable());
  gameList.append(TableViewColumn().setText("MAME Name"));

  auto board = string{};
  if(auto category = categoryList.selected()) board = category.attribute("board");
  auto searchText = searchInput.text();

  for(auto& game : games) {
    if(board && game.board != board) continue;
    if(searchText.size()) {
      if(!game.title.ifind(searchText) &&
         !game.board.ifind(searchText) &&
         !game.name.ifind(searchText)) continue;
    }

    TableViewItem item{&gameList};
    item.setAttribute("name", game.name);
    item.append(TableViewCell().setText(game.title));
    item.append(TableViewCell().setText(game.board));
    item.append(TableViewCell().setText(game.name));
    if(!game.available) {
      for(auto cell : item.cells()) cell.setForegroundColor(Color{255, 0, 0});
    }
  }

  gameList.resizeColumns();
}
