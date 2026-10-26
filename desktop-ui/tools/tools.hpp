#include <chrono>

struct ManifestViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto eventChange() -> void;
  auto setVisible(bool visible = true) -> ManifestViewer&;

  Label manifestLabel{this, Size{~0, 0}, 5};
  ComboButton manifestList{this, Size{~0, 0}};
  TextEdit manifestView{this, Size{~0, ~0}};
};

struct CheatEditor : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto setVisible(bool visible = true) -> CheatEditor&;

  auto find(u32 address) -> maybe<u32>;

  Label cheatsLabel{this, Size{~0, 0}, 5};
  HorizontalLayout editLayout{this, Size{~0, 0}};
    Label descriptionLabel{&editLayout, Size{0, 0}, 2};
    LineEdit descriptionEdit{&editLayout, Size{~0, 0}};
    Label codeLabel{&editLayout, Size{0, 0}, 2};
    LineEdit codeEdit{&editLayout, Size{120, 0}};
    Button saveButton{&editLayout, Size{60, 0}};
  TableView cheatList{this, Size{~0, ~0}};
  HorizontalLayout deleteLayout{this, Size{~0, 0}};
    Button deleteButton{&deleteLayout, Size{80, 0}};


  struct Cheat {
    Cheat() = default;
    auto update(string description, string code, bool enabled = false) -> Cheat&;

    bool enabled;
    string description;
    string code;
    map<u32, u32> addressValuePairs;
  };

  string location;
  std::vector<Cheat> cheats;
};

struct MemoryEditor : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto liveRefresh() -> void;
  auto eventChange() -> void;
  auto eventExport() -> void;
  auto setVisible(bool visible = true) -> MemoryEditor&;

  Label memoryLabel{this, Size{~0, 0}, 5};
  ComboButton memoryList{this, Size{~0, 0}};
  HexEdit memoryEditor{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Button exportButton{&controlLayout, Size{80, 0}};
    Label gotoLabel{&controlLayout, Size{0, 0}, 2};
    LineEdit gotoAddress{&controlLayout, Size{70, 0}};
    Widget spacer{&controlLayout, Size{~0, 0}};
    CheckLabel liveOption{&controlLayout, Size{0, 0}, 2};
    Button refreshButton{&controlLayout, Size{80, 0}};
};

struct GraphicsViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto liveRefresh() -> void;
  auto eventChange() -> void;
  auto eventExport() -> void;
  auto setVisible(bool visible = true) -> GraphicsViewer&;

  Label graphicsLabel{this, Size{~0, 0}, 5};
  ComboButton graphicsList{this, Size{~0, 0}};
  Canvas graphicsView{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Button exportButton{&controlLayout, Size{80, 0}};
    Widget spacer{&controlLayout, Size{~0, 0}};
    CheckLabel liveOption{&controlLayout, Size{0, 0}, 2};
    Button refreshButton{&controlLayout, Size{80, 0}};
};

struct StreamManager : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;

  Label streamLabel{this, Size{~0, 0}, 5};
  TableView streamList{this, Size{~0, ~0}};
};

struct PropertiesViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto liveRefresh() -> void;
  auto eventChange() -> void;
  auto setVisible(bool visible = true) -> PropertiesViewer&;

  Label propertiesLabel{this, Size{~0, 0}, 5};
  ComboButton propertiesList{this, Size{~0, 0}};
  TextEdit propertiesView{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Widget spacer{&controlLayout, Size{~0, 0}};
    CheckLabel liveOption{&controlLayout, Size{0, 0}, 2};
    Button refreshButton{&controlLayout, Size{80, 0}};
};

struct TraceLogger : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;

  file_buffer fp;

  Label tracerLabel{this, Size{~0, 0}, 5};
  TableView tracerList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
};

struct TapeViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto liveRefresh() -> void;
  auto eventChange() -> void;

  bool stopped;

  Label tapeLabel{this, Size{~0, 0}, 5};
  ComboButton tapeList{this, Size{~0, 0}};
  VerticalLayout tapeLayout{this, Size{~0, ~0}};
  Label statusLabel{&tapeLayout, Size{~0, 0}, 5};
  Label lengthLabel{&tapeLayout, Size{~0, 0}, 5};
  HorizontalSlider positionSlider{&tapeLayout, Size{~0, 0}};
  Button newButton{&tapeLayout, Size{~0, 0}};
  Button loadButton{&tapeLayout, Size{~0, 0}};
  Button unloadButton{&tapeLayout, Size{~0, 0}};
  Button playButton{&tapeLayout, Size{~0, 0}};
  Button recordButton{&tapeLayout, Size{~0, 0}};
  Button fastForwardButton{&tapeLayout, Size{~0, 0}};
  Button rewindButton{&tapeLayout, Size{~0, 0}};
  Button stopButton{&tapeLayout, Size{~0, 0}};
};

struct AchievementsViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto liveRefresh() -> void;
  auto eventSubsetChange() -> void;
  auto setVisible(bool visible = true) -> AchievementsViewer&;

  Label title{this, Size{~0, 0}, 5};
  HorizontalLayout subsetLayout{this, Size{~0, 0}, 5};
    Label subsetLabel{&subsetLayout, Size{0, 0}, 2};
    ComboButton subsetList{&subsetLayout, Size{~0, 0}};
  Label summary{this, Size{~0, 0}, 2};
  ProgressBar progress{this, Size{~0, 12}};
  TableView list{this, Size{~0, ~0}};
  RetroAchievementsGameView lastView;
  std::vector<RetroAchievementsGameView::Achievement> rows;
  u32 selectedSetId = 0;
  u64 lastImageVersion = 0;
  std::chrono::steady_clock::time_point lastPoll{};
  bool updating = false;
};

struct LeaderboardsViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto liveRefresh() -> void;
  auto eventLeaderboardChange() -> void;
  auto setVisible(bool visible = true) -> LeaderboardsViewer&;

  Label title{this, Size{~0, 0}, 5};
  Label status{this, Size{~0, 0}, 2};
  HorizontalLayout content{this, Size{~0, ~0}, 5};
    ListView list{&content, Size{220_sx, ~0}};
    VerticalLayout detail{&content, Size{~0, ~0}, 5};
      Label selectedTitle{&detail, Size{~0, 0}};
      Label description{&detail, Size{~0, 0}};
      Label mode{&detail, Size{~0, 0}};
      HorizontalLayout controls{&detail, Size{~0, 0}, 5};
        Label fetchStatus{&controls, Size{~0, 0}};
        Button aroundButton{&controls, Size{100, 0}};
        Button topButton{&controls, Size{100, 0}};
      TableView entries{&detail, Size{~0, ~0}};
  RetroAchievementsGameView lastView;
  RetroAchievementsLeaderboardView lastEntries;
  u32 selectedId = 0;
  u32 requestedId = 0;
  bool topEntries = false;
  bool updating = false;
  u64 lastImageVersion = 0;
  std::chrono::steady_clock::time_point lastPoll{};
};

struct ChallengesViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto setVisible(bool visible = true) -> ChallengesViewer&;

  Label title{this, Size{~0, 0}, 5};
  TextEdit view{this, Size{~0, ~0}};
};

struct ToolsWindow : Window {
  ToolsWindow();
  auto show(const string& panel) -> void;
  auto eventChange() -> void;

  HorizontalLayout layout{this};
    ListView panelList{&layout, Size{125_sx, ~0}};
    VerticalLayout panelContainer{&layout, Size{~0, ~0}};
      ManifestViewer manifestViewer;
      CheatEditor cheatEditor;
      MemoryEditor memoryEditor;
      GraphicsViewer graphicsViewer;
      StreamManager streamManager;
      PropertiesViewer propertiesViewer;
      TraceLogger traceLogger;
      TapeViewer tapeViewer;
      AchievementsViewer achievementsViewer;
      LeaderboardsViewer leaderboardsViewer;
      ChallengesViewer challengesViewer;
      HomePanel homePanel;
};

namespace Instances { extern Instance<ToolsWindow> toolsWindow; }
extern ToolsWindow& toolsWindow;
extern CheatEditor& cheatEditor;
extern ManifestViewer& manifestViewer;
extern MemoryEditor& memoryEditor;
extern GraphicsViewer& graphicsViewer;
extern StreamManager& streamManager;
extern PropertiesViewer& propertiesViewer;
extern TraceLogger& traceLogger;
extern TapeViewer& tapeViewer;
extern AchievementsViewer& achievementsViewer;
extern LeaderboardsViewer& leaderboardsViewer;
extern ChallengesViewer& challengesViewer;
