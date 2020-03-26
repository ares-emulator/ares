#include "../luna.hpp"
#include "../panel-items/home.cpp"
#include "../panel-items/system-creation.cpp"
#include "../panel-items/system-overview.cpp"
#include "../panel-items/port-connector.cpp"
#include "../panel-items/peripheral-overview.cpp"
#include "../panel-items/input-mapper.cpp"
#include "../panel-items/setting-editor.cpp"
#include "../panel-items/video.cpp"
#include "../panel-items/audio.cpp"
#include "../panel-items/input.cpp"
#include "../panel-items/hotkeys.cpp"
#include "../panel-lists/settings-manager.cpp"
#include "../panel-lists/system-manager.cpp"
#include "../panel-lists/node-manager.cpp"
#include "../panel-lists/port-manager.cpp"
#include "../panel-lists/event-manager.cpp"

namespace Instances { Instance<Program> program; }
Program& program = Instances::program();
ActionMenu& actionMenu = program.actionMenu;
SystemMenu& systemMenu = program.systemMenu;
SettingsMenu& settingsMenu = program.settingsMenu;
ToolsMenu& toolsMenu = program.toolsMenu;
HelpMenu& helpMenu = program.helpMenu;
SettingsManager& settingsManager = program.settingsManager;
SystemManager& systemManager = program.systemManager;
NodeManager& nodeManager = program.nodeManager;
PortManager& portManager = program.portManager;
EventManager& eventManager = program.eventManager;
Home& home = program.home;
SystemCreation& systemCreation = program.systemCreation;
SystemOverview& systemOverview = program.systemOverview;
PortConnector& portConnector = program.portConnector;
PeripheralOverview& peripheralOverview = program.peripheralOverview;
InputMapper& inputMapper = program.inputMapper;
SettingEditor& settingEditor = program.settingEditor;
VideoSettings& videoSettings = program.videoSettings;
AudioSettings& audioSettings = program.audioSettings;
InputSettings& inputSettings = program.inputSettings;
HotkeySettings& hotkeySettings = program.hotkeySettings;

Program::Program() {
  viewport.setCollapsible().setFocusable();

  statusHeight = Font().size(" ").height() + 8_sy;
  layout.cell(statusLayout).setSize({~0, statusHeight});

  statusLayout.setCollapsible().setVisible(settings.interface.showStatusBar);
  statusMessage.setFont(Font().setBold());
  statusCaption.setFont(Font().setBold()).setAlignment(1.0);

  Color foreground{255, 255, 255};
  Color background{ 48,  48,  48};
  statusBefore.setForegroundColor(foreground).setBackgroundColor(background);
  statusMessage.setForegroundColor(foreground).setBackgroundColor(background);
  statusCaption.setForegroundColor(foreground).setBackgroundColor(background);
  statusAfter.setForegroundColor(foreground).setBackgroundColor(background);

  panelGroup.onChange([&] {
    if(auto item = panelGroup.selected()) {
      if(auto list = item.attribute<PanelList*>("panelList")) {
        setPanelList(*list);
      }
    }
  });

  panelHeight = 7 + 250_sy;
  verticalResizeGrip.setCollapsible().setVisible(settings.interface.showSystemPanels);
  panelLayout.setCollapsible().setVisible(settings.interface.showSystemPanels);
  panelLayout.setPadding({5_sx, 0_sy, 5_sx, 5_sy});

  for(auto& cell : panelLayout.cells()) cell.setSpacing(0);

  verticalResizeGrip.onActivate([&] {
    verticalResizeHeight = layout.cell(panelLayout).size().height();
  }).onResize([&](auto offset) {
    float min = 128_sy, max = geometry().height() - 128_sy;
    float height = verticalResizeHeight - offset;
    height = height < min ? min : height > max ? max : height;
    if(layout.cell(panelLayout).size().height() != height) {
      layout.cell(panelLayout).setSize({~0, height});
      layout.resize();
    }
  });

  horizontalResizeGrip.onActivate([&] {
    horizontalResizeWidth = panelLayout.cell(panelBlock).size().width();
  }).onResize([&](auto offset) {
    float min = 128_sx, max = panelLayout.geometry().width() - 128_sx;
    float width = horizontalResizeWidth + offset;
    width = width < min ? min : width > max ? max : width;
    if(panelLayout.cell(panelBlock).size().width() != width) {
      panelLayout.cell(panelBlock).setSize({width, ~0});
      panelLayout.resize();
    }
  });

  onClose([&] { emulator.quit(); });

  setOverviewMode();
  resize();

  setTitle({"luna v", ares::Version});
  setVisible();

  emulator.inputUpdate();
  videoSettings.eventChange();
  audioSettings.eventChange();
  inputSettings.eventChange();
}

auto Program::resize() -> void {
  uint scale  = max(2, min(4, settings.video.scale));
  uint width  = 320_sx * scale;
  uint height = 240_sx * scale;
  if(settings.interface.showStatusBar) height += statusHeight;
  if(settings.interface.showSystemPanels) height += panelHeight;
  setGeometry(Alignment::Center, {width, height});
}

auto Program::setOverviewMode() -> void {
  if(panelGroup.attribute("mode") == "overview") return;
  panelGroup.setAttribute("mode", "overview");
  panelGroup.reset();
  panelGroup.append(ComboButtonItem().setText("Systems").setAttribute<PanelList*>("panelList", &systemManager));
  panelGroup.append(ComboButtonItem().setText("Settings").setAttribute<PanelList*>("panelList", &settingsManager));
  setPanelList(systemManager);
  setPanelItem(home);

  systemMenu.setVisible(false);
  toolsMenu.setVisible(false);
  //todo: if test shouldn't be necessary ... works around a hiro/GTK bug ...
  if(!actionMenu.visible()) actionMenu.setVisible(true);
}

auto Program::setEmulatorMode() -> void {
  if(panelGroup.attribute("mode") == "emulator") return;
  panelGroup.setAttribute("mode", "emulator");
  panelGroup.reset();
  panelGroup.append(ComboButtonItem().setText(emulator.system.name).setAttribute<PanelList*>("panelList", &nodeManager));
  panelGroup.append(ComboButtonItem().setText("Ports").setAttribute<PanelList*>("panelList", &portManager));
  panelGroup.append(ComboButtonItem().setText("Events").setAttribute<PanelList*>("panelList", &eventManager));
  panelGroup.append(ComboButtonItem().setText("Settings").setAttribute<PanelList*>("panelList", &settingsManager));
  setPanelList(nodeManager);
  setPanelItem(home);

  actionMenu.setVisible(false);
  systemMenu.setVisible(true);
  toolsMenu.setVisible(true);
}

auto Program::setPanelList(PanelList& panelList) -> void {
  if(activePanelList && activePanelList() == panelList) return;
  if(activePanelList) activePanelList().hide();
  setPanelItem(home);
  activePanelList = panelList;
  activePanelList().show();
  panelBlock.resize();

  //update the panel group selection as well
  for(auto item : panelGroup.items()) {
    if(item.attribute<PanelList*>("panelList") == &panelList) {
      item.setSelected();
    }
  }
}

auto Program::setPanelItem(PanelItem& panelItem) -> void {
  if(activePanelItem && activePanelItem() == panelItem) return;
  if(activePanelItem) activePanelItem().hide();
  activePanelItem = panelItem;
  activePanelItem().show();
  panelLayout.resize();
  //todo: this works around a Windows rendering issue on the audio settings panel, but should not be needed:
  Application::processEvents();
  panelLayout.resize();
}

auto Program::refreshPanelList() -> void {
  if(activePanelList) activePanelList().refresh();
}

auto Program::showStatus() -> void {
  if(statusLayout.visible()) return;
  statusLayout.setVisible(true);
  if(!maximized()) setSize({geometry().width(), geometry().height() + statusHeight});
  layout.resize();
  settingsMenu.showStatusBar.setChecked(true);
}

auto Program::hideStatus() -> void {
  if(!statusLayout.visible()) return;
  statusLayout.setVisible(false);
  if(!maximized()) setSize({geometry().width(), geometry().height() - statusHeight});
  layout.resize();
  settingsMenu.showStatusBar.setChecked(false);
}

auto Program::showPanels() -> void {
  if(panelLayout.visible()) return;
  verticalResizeGrip.setVisible(true);
  panelLayout.setVisible(true);
  if(!maximized()) setSize({geometry().width(), geometry().height() + panelHeight});
  layout.resize();
  settingsMenu.showSystemPanels.setChecked(true);
}

auto Program::hidePanels() -> void {
  if(!panelLayout.visible()) return;
  panelHeight = 7 + panelLayout.geometry().height();
  verticalResizeGrip.setVisible(false);
  panelLayout.setVisible(false);
  if(!maximized()) setSize({geometry().width(), geometry().height() - panelHeight});
  layout.resize();
  settingsMenu.showSystemPanels.setChecked(false);
}
