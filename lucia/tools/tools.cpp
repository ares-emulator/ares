#include "../lucia.hpp"
#include "manifest.cpp"
#include "memory.cpp"
#include "tracer.cpp"

namespace Instances { Instance<ToolsWindow> toolsWindow; }
ToolsWindow& toolsWindow = Instances::toolsWindow();
ManifestViewer& manifestViewer = toolsWindow.manifestViewer;
MemoryEditor& memoryEditor = toolsWindow.memoryEditor;
TraceLogger& traceLogger = toolsWindow.traceLogger;

ToolsWindow::ToolsWindow() {
  layout.setPadding(5_sx, 5_sy);

  panelList.append(ListViewItem().setText("Manifest").setIcon(Icon::Emblem::Binary));
  panelList.append(ListViewItem().setText("Memory").setIcon(Icon::Device::Storage));
  panelList.append(ListViewItem().setText("Tracer").setIcon(Icon::Emblem::Text));
  panelList.onChange([&] { eventChange(); });

  panelContainer.append(manifestViewer, Size{~0, ~0});
  panelContainer.append(memoryEditor, Size{~0, ~0});
  panelContainer.append(traceLogger, Size{~0, ~0});
  panelContainer.append(homePanel, Size{~0, ~0});

  manifestViewer.construct();
  memoryEditor.construct();
  traceLogger.construct();
  homePanel.construct();

  setDismissable();
  setTitle("Tools");
  setSize({690_sx, 405_sy});
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
  memoryEditor.setVisible(false);
  traceLogger.setVisible(false);
  homePanel.setVisible(false);

  bool found = false;
  if(auto item = panelList.selected()) {
    if(item.text() == "Manifest") found = true, manifestViewer.setVisible();
    if(item.text() == "Memory") found = true, memoryEditor.setVisible();
    if(item.text() == "Tracer") found = true, traceLogger.setVisible();
  }
  if(!found) homePanel.setVisible();

  panelContainer.resize();
}
