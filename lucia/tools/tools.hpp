struct ManifestViewer : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;

  Label manifestLabel{this, Size{~0, 0}, 2};
  TextEdit manifestView{this, Size{~0, ~0}};
};

struct TraceLogger : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto unload() -> void;
  auto eventToggle(ListViewItem) -> void;

  file_buffer fp;

  Label tracerLabel{this, Size{~0, 0}, 2};
  ListView tracerList{this, Size{~0, ~0}};
};

struct ToolsWindow : Window {
  ToolsWindow();
  auto show(const string& panel) -> void;
  auto eventChange() -> void;

  HorizontalLayout layout{this};
    ListView panelList{&layout, Size{125_sx, ~0}};
    VerticalLayout panelContainer{&layout, Size{~0, ~0}};
      ManifestViewer manifestViewer;
      TraceLogger traceLogger;
      HomePanel homePanel;
};

namespace Instances { extern Instance<ToolsWindow> toolsWindow; }
extern ToolsWindow& toolsWindow;
extern ManifestViewer& manifestViewer;
extern TraceLogger& traceLogger;
