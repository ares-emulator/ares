struct Presentation : Window {
  enum : u32 { StatusHeight = 24 };

  Presentation();
  auto resizeWindow() -> void;
  auto loadEmulators() -> void;
  auto loadEmulator() -> void;
  auto unloadEmulator(bool reloading = false) -> void;
  auto showIcon(bool visible) -> void;
  auto loadShaders() -> void;
  auto refreshSystemMenu() -> void;

  MenuBar menuBar{this};
    Menu loadMenu{&menuBar};
    Menu systemMenu{&menuBar};
    Menu settingsMenu{&menuBar};
      Menu videoSizeMenu{&settingsMenu};
        Group videoSizeGroup;
      Menu videoOutputMenu{&settingsMenu};
        MenuRadioItem videoOutputPixelPerfect{&videoOutputMenu};
        MenuRadioItem videoOutputFixedScale{&videoOutputMenu};
        MenuRadioItem videoOutputIntegerScale{&videoOutputMenu};
        MenuRadioItem videoOutputScale{&videoOutputMenu};
        MenuRadioItem videoOutputStretch{&videoOutputMenu};
        Group videoOutputGroup{&videoOutputPixelPerfect, &videoOutputFixedScale, &videoOutputIntegerScale,
                               &videoOutputScale, &videoOutputStretch};
        MenuSeparator videoOutputSeparator{&videoOutputMenu};
        MenuCheckItem videoAspectCorrection{&videoOutputMenu};
        MenuCheckItem videoAdaptiveSizing{&videoOutputMenu};
        MenuCheckItem videoAutoCentering{&videoOutputMenu};
      Menu videoShaderMenu{&settingsMenu};
      Menu bootOptionsMenu{&settingsMenu};
        MenuCheckItem fastBoot{&bootOptionsMenu};
        MenuCheckItem launchDebugger{&bootOptionsMenu};
        MenuSeparator bootOptionsSeparator{&bootOptionsMenu};
        MenuRadioItem preferNTSCU{&bootOptionsMenu};
        MenuRadioItem preferNTSCJ{&bootOptionsMenu};
        MenuRadioItem preferPAL{&bootOptionsMenu};
        Group preferRegionGroup{&preferNTSCU, &preferNTSCJ, &preferPAL};
      MenuSeparator groupSettingsSeparatpr{&settingsMenu};
      MenuCheckItem muteAudioSetting{&settingsMenu};
      MenuCheckItem showStatusBarSetting{&settingsMenu};
      MenuSeparator audioSettingsSeparator{&settingsMenu};
      MenuItem videoSettingsAction{&settingsMenu};
      MenuItem audioSettingsAction{&settingsMenu};
      MenuItem inputSettingsAction{&settingsMenu};
      MenuItem hotkeySettingsAction{&settingsMenu};
      MenuItem emulatorSettingsAction{&settingsMenu};
      MenuItem optionSettingsAction{&settingsMenu};
      MenuItem firmwareSettingsAction{&settingsMenu};
      MenuItem pathSettingsAction{&settingsMenu};
      MenuItem driverSettingsAction{&settingsMenu};
      MenuItem debugSettingsAction{&settingsMenu};
    Menu toolsMenu{&menuBar};
      Menu saveStateMenu{&toolsMenu};
      Menu loadStateMenu{&toolsMenu};
      MenuItem undoSaveStateMenu{&toolsMenu};
      MenuItem undoLoadStateMenu{&toolsMenu};
      MenuItem captureScreenshot{&toolsMenu};
      MenuSeparator toolsMenuSeparatorA{&toolsMenu};
      MenuCheckItem pauseEmulation{&toolsMenu};
      MenuItem frameAdvance{&toolsMenu};
      MenuItem reloadGame{&toolsMenu};
      MenuSeparator toolsMenuSeparatorB{&toolsMenu};
      MenuItem manifestViewerAction{&toolsMenu};
      MenuItem cheatEditorAction{&toolsMenu};
      #if !defined(PLATFORM_MACOS)
      // Cocoa hiro is missing the hex editor widget
      MenuItem memoryEditorAction{&toolsMenu};
      #endif
      MenuItem graphicsViewerAction{&toolsMenu};
      MenuItem streamManagerAction{&toolsMenu};
      MenuItem propertiesViewerAction{&toolsMenu};
      MenuItem traceLoggerAction{&toolsMenu};
    Menu helpMenu{&menuBar};
      MenuItem aboutAction{&helpMenu};

  VerticalLayout layout{this};
    HorizontalLayout viewportLayout{&layout, Size{~0, ~0}, 0};
      Viewport viewport{&viewportLayout, Size{~0, ~0}, 0};
      VerticalLayout iconLayout{&viewportLayout, Size{144, ~0}, 0};
        Canvas iconSpacer{&iconLayout, Size{144, ~0}, 0};
        HorizontalLayout iconHorizontal{&iconLayout, Size{144, 128}, 0};
          Canvas iconCanvas{&iconHorizontal, Size{128, 128}, 0};
          Canvas iconPadding{&iconHorizontal, Size{16, 128}, 0};
        Canvas iconBottom{&iconLayout, Size{144, 10}, 0};
    HorizontalLayout statusLayout{&layout, Size{~0, StatusHeight}, 0};
      Label spacerLeft{&statusLayout, Size{8, ~0}, 0};
      Label statusLeft{&statusLayout, Size{~0, ~0}, 0};
      Label statusDebug{&statusLayout, Size{200, ~0}, 0};
      Label statusRight{&statusLayout, Size{90, ~0}, 0};
      Label spacerRight{&statusLayout, Size{8, ~0}, 0};
};

namespace Instances { extern Instance<Presentation> presentation; }
extern Presentation& presentation;
