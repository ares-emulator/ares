#include "../lucia.hpp"
#include "manifest.cpp"
#include "tracer.cpp"

namespace Instances { Instance<ToolsWindow> toolsWindow; }
ToolsWindow& toolsWindow = Instances::toolsWindow();
ManifestViewer& manifestViewer = toolsWindow.manifestViewer;
TraceLogger& traceLogger = toolsWindow.traceLogger;

ToolsWindow::ToolsWindow() {
  layout.setPadding(5_sx, 5_sy);

  panelList.append(ListViewItem().setText("Manifest").setIcon(Icon::Emblem::Text));
  panelList.append(ListViewItem().setText("Tracer").setIcon(Icon::Emblem::Script));
  panelList.onChange([&] { eventChange(); });

  panelContainer.append(manifestViewer, Size{~0, ~0});
  panelContainer.append(traceLogger, Size{~0, ~0});
  panelContainer.append(homePanel, Size{~0, ~0});

  manifestViewer.construct();
  traceLogger.construct();
  homePanel.construct();

  setDismissable();
  setTitle("Tools");
  setSize({720, 400});
  setAlignment({1.0, 1.0});
}

auto ToolsWindow::show(const string& panel) -> void {
  for(auto& item : panelList.items()) {
    if(item.text() == panel) {
      item.setSelected();
      eventChange();
      break;
    }
  }
  setVisible();
  setFocused();
  panelList.setFocused();
}

auto ToolsWindow::eventChange() -> void {
  manifestViewer.setVisible(false);
  traceLogger.setVisible(false);
  homePanel.setVisible(false);

  bool found = false;
  if(auto item = panelList.selected()) {
    if(item.text() == "Manifest") found = true, manifestViewer.setVisible();
    if(item.text() == "Tracer") found = true, traceLogger.setVisible();
  }
  if(!found) homePanel.setVisible();

  panelContainer.resize();
}
