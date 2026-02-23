struct Panel : VerticalLayout {
  using VerticalLayout::VerticalLayout;
};

using View = HorizontalLayout;

#include "system-selection.hpp"
#include "home.hpp"
#include "game-manager.hpp"
#include "game-importer.hpp"

struct ProgramWindow : Window {
  ProgramWindow();
  auto show(Panel&) -> void;
  auto hide(Panel&) -> void;

  MenuBar menuBar{this};
    Menu actionMenu{&menuBar};
      MenuItem quitAction{&actionMenu};
    Menu settingsMenu{&menuBar};
      MenuCheckItem createManifests{&settingsMenu};
      MenuCheckItem useDatabase{&settingsMenu};
      MenuCheckItem useHeuristics{&settingsMenu};
    Menu helpMenu{&menuBar};
      MenuItem aboutAction{&helpMenu};

  HorizontalLayout panels{this};
    SystemSelection systemSelection{&panels};
    HorizontalResizeGrip resizeGrip{&panels, Size{7, ~0}};
    Home home{&panels};
    GameManager gameManager{&panels};
    GameImporter gameImporter{&panels};

  maybe<Panel&> activePanel;
  float resizeWidth = 0;
};

namespace Instances { extern Instance<ProgramWindow> programWindow; }
extern ProgramWindow& programWindow;
