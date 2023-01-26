struct Presentation : Window {
  enum : u32 { StatusHeight = 24 };

  Presentation();
  auto resizeWindow() -> void;
  auto loadEmulators() -> void;
  auto loadEmulator() -> void;
  auto unloadEmulator(bool reloading = false) -> void;
  auto showIcon(bool visible) -> void;
  auto loadShaders() -> void;

  MenuBar menuBar{this};
    Menu loadMenu{&menuBar};
    Menu systemMenu{&menuBar};
    Menu settingsMenu{&menuBar};
      Menu videoScaleMenu{&settingsMenu};
        MenuRadioItem videoScaleOne{&videoScaleMenu};
        MenuRadioItem videoScaleTwo{&videoScaleMenu};
        MenuRadioItem videoScaleThree{&videoScaleMenu};
        MenuRadioItem videoScaleFour{&videoScaleMenu};
        MenuRadioItem videoScaleFive{&videoScaleMenu};
        Group videoScaleGroup{&videoScaleOne, &videoScaleTwo, &videoScaleThree, &videoScaleFour, &videoScaleFive};
      Menu videoOutputMenu{&settingsMenu};
        MenuRadioItem videoOutputCenter{&videoOutputMenu};
        MenuRadioItem videoOutputScale{&videoOutputMenu};
        MenuRadioItem videoOutputStretch{&videoOutputMenu};
        Group videoOutputGroup{&videoOutputCenter, &videoOutputScale, &videoOutputStretch};
        MenuSeparator videoOutputSeparator{&videoOutputMenu};
        MenuCheckItem videoAspectCorrection{&videoOutputMenu};
        MenuCheckItem videoAdaptiveSizing{&videoOutputMenu};
      Menu videoShaderMenu{&settingsMenu};
      Menu bootOptionsMenu{&settingsMenu};
        MenuCheckItem fastBoot{&bootOptionsMenu};
        MenuCheckItem launchDebugger{&bootOptionsMenu};
        MenuSeparator bootOptionsSeparator{&bootOptionsMenu};
        MenuRadioItem preferNTSCU{&bootOptionsMenu};
        MenuRadioItem preferNTSCJ{&bootOptionsMenu};
        MenuRadioItem preferPAL{&bootOptionsMenu};
        Group preferRegionGroup{&preferNTSCU, &preferNTSCJ, &preferPAL};
      MenuSeparator groupSettingsSeparator{&settingsMenu};
      MenuCheckItem muteAudioSetting{&settingsMenu};
      MenuCheckItem showStatusBarSetting{&settingsMenu};
      MenuSeparator audioSettingsSeparator{&settingsMenu};
      MenuItem videoSettingsAction{&settingsMenu};
      MenuItem audioSettingsAction{&settingsMenu};
      MenuItem inputSettingsAction{&settingsMenu};
      MenuItem hotkeySettingsAction{&settingsMenu};
      MenuItem optionSettingsAction{&settingsMenu};
      MenuItem firmwareSettingsAction{&settingsMenu};
      MenuItem pathSettingsAction{&settingsMenu};
      MenuItem driverSettingsAction{&settingsMenu};
    Menu toolsMenu{&menuBar};
      Menu saveStateMenu{&toolsMenu};
      Menu loadStateMenu{&toolsMenu};
      MenuItem captureScreenshot{&toolsMenu};
      MenuSeparator toolsMenuSeparatorA{&toolsMenu};
      MenuCheckItem pauseEmulation{&toolsMenu};
      MenuItem frameAdvance{&toolsMenu};
      MenuItem reloadGame{&toolsMenu};
      MenuSeparator toolsMenuSeparatorB{&toolsMenu};
      MenuItem manifestViewerAction{&toolsMenu};
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
      VerticalLayout iconLayout{&viewportLayout, Size{128, ~0}, 0};
        Canvas iconSpacer{&iconLayout, Size{128, ~0}, 0};
        HorizontalLayout iconHorizontal{&iconLayout, Size{128, 128}, 0};
          Canvas iconCanvas{&iconHorizontal, Size{112, 128}, 0};
          Canvas iconPadding{&iconHorizontal, Size{16, 128}, 0};
        Canvas iconBottom{&iconLayout, Size{128, 10}, 0};
    HorizontalLayout statusLayout{&layout, Size{~0, StatusHeight}, 0};
      Label spacerLeft{&statusLayout, Size{8, ~0}, 0};
      Label statusLeft{&statusLayout, Size{~0, ~0}, 0};
      Label statusRight{&statusLayout, Size{100, ~0}, 0};
      Label spacerRight{&statusLayout, Size{8, ~0}, 0};
};

namespace Instances { extern Instance<Presentation> presentation; }
extern Presentation& presentation;
