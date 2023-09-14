struct Settings : Markup::Node {
  using string = nall::string;

  auto load() -> void;
  auto save() -> void;
  auto process(bool load) -> void;

  struct Video {
    string driver;
    string monitor;
    string format;
    bool exclusive = false;
    bool blocking = false;
    bool flush = false;
    string shader = "None";
    u32 multiplier = 2;
    string output = "Scale";
    bool aspectCorrection = true;
    bool adaptiveSizing = true;
    bool autoCentering = false;

    f64 luminance = 1.0;
    f64 saturation = 1.0;
    f64 gamma = 1.0;
    bool colorBleed = false;
    bool colorEmulation = true;
    bool interframeBlending = true;
    bool overscan = false;
    bool pixelAccuracy = false;

    string quality = "SD";
    bool supersampling = false;
    bool disableVideoInterfaceProcessing = false;
    bool weaveDeinterlacing = true;
  } video;

  struct Audio {
    string driver;
    string device;
    u32 frequency = 0;
    u32 latency = 0;
    bool exclusive = false;
    bool blocking = true;
    bool dynamic = false;
    bool mute = false;

    f64 volume = 1.0;
    f64 balance = 0.0;
  } audio;

  struct Input {
    string driver;
    string defocus = "Pause";
  } input;

  struct Boot {
    bool fast = false;
    bool debugger = false;
    string prefer = "NTSC-U";
  } boot;

  struct General {
    bool showStatusBar = true;
    bool rewind = false;
    bool runAhead = false;
    bool autoSaveMemory = true;
  } general;

  struct Rewind {
    u32 length = 100;
    u32 frequency = 10;
  } rewind;

  struct Paths {
    string home;
    string firmware;
    string saves;
    string screenshots;
    string debugging;
    string arcadeRoms;
    struct SuperFamicom {
      string gameBoy;
      string bsMemory;
      string sufamiTurbo;
    } superFamicom;
  } paths;

  struct Recent {
    string game[9];
  } recent;

  struct DebugServer {
    u32 port = 9123;
    bool enabled = false; // if enabled, server starts with ares
    bool useIPv4 = false; // forces IPv4 over IPv6
  } debugServer;
};

struct VideoSettings : VerticalLayout {
  auto construct() -> void;

  Label colorAdjustmentLabel{this, Size{~0, 0}, 5};
  TableLayout colorAdjustmentLayout{this, Size{~0, 0}};
    Label luminanceLabel{&colorAdjustmentLayout, Size{0, 0}};
    Label luminanceValue{&colorAdjustmentLayout, Size{50_sx, 0}};
    HorizontalSlider luminanceSlider{&colorAdjustmentLayout, Size{~0, 0}};
  //
    Label saturationLabel{&colorAdjustmentLayout, Size{0, 0}};
    Label saturationValue{&colorAdjustmentLayout, Size{50_sx, 0}};
    HorizontalSlider saturationSlider{&colorAdjustmentLayout, Size{~0, 0}};
  //
    Label gammaLabel{&colorAdjustmentLayout, Size{0, 0}};
    Label gammaValue{&colorAdjustmentLayout, Size{50_sx, 0}};
    HorizontalSlider gammaSlider{&colorAdjustmentLayout, Size{~0, 0}};
  Label emulatorSettingsLabel{this, Size{~0, 0}, 5};
    HorizontalLayout colorBleedLayout{this, Size{~0, 0}, 5};
      CheckLabel colorBleedOption{&colorBleedLayout, Size{0, 0}, 5};
      Label colorBleedHint{&colorBleedLayout, Size{~0, 0}};
    HorizontalLayout colorEmulationLayout{this, Size{~0, 0}, 5};
      CheckLabel colorEmulationOption{&colorEmulationLayout, Size{0, 0}, 5};
      Label colorEmulationHint{&colorEmulationLayout, Size{~0, 0}};
    HorizontalLayout interframeBlendingLayout{this, Size{~0, 0}, 5};
      CheckLabel interframeBlendingOption{&interframeBlendingLayout, Size{0, 0}, 5};
      Label interframeBlendingHint{&interframeBlendingLayout, Size{~0, 0}};
    HorizontalLayout overscanLayout{this, Size{~0, 0}};
      CheckLabel overscanOption{&overscanLayout, Size{0, 0}, 5};
      Label overscanHint{&overscanLayout, Size{~0, 0}};
    HorizontalLayout pixelAccuracyLayout{this, Size{~0, 0}};
      CheckLabel pixelAccuracyOption{&pixelAccuracyLayout, Size{0, 0}, 5};
      Label pixelAccuracyHint{&pixelAccuracyLayout, Size{~0, 0}};
  //
  Label renderSettingsLabel{this, Size{~0, 0}, 5};
  HorizontalLayout disableVideoInterfaceProcessingLayout{this, Size{~0, 0}, 5};
    CheckLabel disableVideoInterfaceProcessingOption{&disableVideoInterfaceProcessingLayout, Size{0, 0}, 5};
    Label disableVideoInterfaceProcessingHint{&disableVideoInterfaceProcessingLayout, Size{0, 0}};
  HorizontalLayout weaveDeinterlacingLayout{this, Size{~0, 0}, 5};
    CheckLabel weaveDeinterlacingOption{&weaveDeinterlacingLayout, Size{0, 0}, 5};
    Label weaveDeinterlacingHint{&weaveDeinterlacingLayout, Size{0, 0}};
  HorizontalLayout renderQualityLayout{this, Size{~0, 0}, 5};
    RadioLabel renderQualitySD{&renderQualityLayout, Size{0, 0}};
    RadioLabel renderQualityHD{&renderQualityLayout, Size{0, 0}};
    RadioLabel renderQualityUHD{&renderQualityLayout, Size{0, 0}};
    Group renderQualityGroup{&renderQualitySD, &renderQualityHD, &renderQualityUHD};
  HorizontalLayout renderSupersamplingLayout{this, Size{~0, 0}, 5};
    CheckLabel renderSupersamplingOption{&renderSupersamplingLayout, Size{0, 0}, 5};
    Label renderSupersamplingHint{&renderSupersamplingLayout, Size{0, 0}};
  HorizontalLayout renderSettingsLayout{this, Size{~0, 0}};
      Label renderSettingsHint{&renderSettingsLayout, Size{0, 0}};
};

struct AudioSettings : VerticalLayout {
  auto construct() -> void;

  Label effectsLabel{this, Size{~0, 0}, 5};
  TableLayout effectsLayout{this, Size{~0, 0}};
    Label volumeLabel{&effectsLayout, Size{0, 0}};
    Label volumeValue{&effectsLayout, Size{50_sx, 0}};
    HorizontalSlider volumeSlider{&effectsLayout, Size{~0, 0}};
  //
    Label balanceLabel{&effectsLayout, Size{0, 0}};
    Label balanceValue{&effectsLayout, Size{50_sx, 0}};
    HorizontalSlider balanceSlider{&effectsLayout, Size{~0, 0}};
};

struct InputSettings : VerticalLayout {
  auto construct() -> void;
  auto systemChange() -> void;
  auto portChange() -> void;
  auto deviceChange() -> void;
  auto refresh() -> void;
  auto eventContext(TableViewCell) -> void;
  auto eventChange() -> void;
  auto eventClear() -> void;
  auto eventAssign(TableViewCell, string binding) -> void;
  auto eventAssign(TableViewCell) -> void;
  auto eventInput(shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void;
  auto setVisible(bool visible = true) -> InputSettings&;

  HorizontalLayout indexLayout{this, Size{~0, 0}};
    ComboButton systemList{&indexLayout, Size{~0, 0}};
    ComboButton portList{&indexLayout, Size{~0, 0}};
    ComboButton deviceList{&indexLayout, Size{~0, 0}};
  TableView inputList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Label assignLabel{&controlLayout, Size{~0, 0}};
    Canvas spacer{&controlLayout, Size{1, 0}};
    Button assignButton{&controlLayout, Size{80, 0}};
    Button clearButton{&controlLayout, Size{80, 0}};

  maybe<InputNode&> activeMapping;
  u32 activeBinding = 0;
  Timer timer;
  PopupMenu menu;
};

struct HotkeySettings : VerticalLayout {
  auto construct() -> void;
  auto reload() -> void;
  auto refresh() -> void;
  auto eventChange() -> void;
  auto eventClear() -> void;
  auto eventAssign(TableViewCell) -> void;
  auto eventInput(shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void;
  auto setVisible(bool visible = true) -> HotkeySettings&;

  TableView inputList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Label assignLabel{&controlLayout, Size{~0, 0}};
    Canvas spacer{&controlLayout, Size{1, 0}};
    Button assignButton{&controlLayout, Size{80, 0}};
    Button clearButton{&controlLayout, Size{80, 0}};

  maybe<InputHotkey&> activeMapping;
  u32 activeBinding = 0;
  Timer timer;
};

struct EmulatorSettings : VerticalLayout {
  auto construct() -> void;
  auto eventToggle(TableViewCell cell) -> void;

  Label emulatorLabel{this, Size{~0, 0}, 5};
  TableView emulatorList{this, Size{~0, ~0}};
};

struct OptionSettings : VerticalLayout {
  auto construct() -> void;
  HorizontalLayout rewindLayout{this, Size{~0, 0}, 5};
    CheckLabel rewind{&rewindLayout, Size{0, 0}, 5};
    Label rewindHint{&rewindLayout, Size{~0, 0}};
  HorizontalLayout runAheadLayout{this, Size{~0, 0}, 5};
    CheckLabel runAhead{&runAheadLayout, Size{0, 0}, 5};
    Label runAheadHint{&runAheadLayout, Size{~0, 0}};
  HorizontalLayout autoSaveMemoryLayout{this, Size{~0, 0}, 5};
    CheckLabel autoSaveMemory{&autoSaveMemoryLayout, Size{0, 0}, 5};
    Label autoSaveMemoryHint{&autoSaveMemoryLayout, Size{~0, 0}};
};

struct FirmwareSettings : VerticalLayout {
  auto construct() -> void;
  auto refresh() -> void;
  auto select(const string& emulator, const string& type, const string& region) -> bool;
  auto eventChange() -> void;
  auto eventAssign() -> void;
  auto eventClear() -> void;
  auto eventScan() -> void;
  auto findFirmware(string sha256) -> string;

  Label firmwareLabel{this, Size{~0, 0}, 5};
  TableView firmwareList{this, Size{~0, ~0}};
  HorizontalLayout controlLayout{this, Size{~0, 0}};
    Button scanButton{&controlLayout, Size{80, 0}};
    Canvas spacer{&controlLayout, Size{~0, 0}};
    Button assignButton{&controlLayout, Size{80, 0}};
    Button clearButton{&controlLayout, Size{80, 0}};

  map<string, string> fileHashes;
};

struct PathSettings : VerticalLayout {
  auto construct() -> void;
  auto refresh() -> void;

  Label homeLabel{this, Size{~0, 0}, 5};
  HorizontalLayout homeLayout{this, Size{~0, 0}};
    LineEdit homePath{&homeLayout, Size{~0, 0}};
    Button homeAssign{&homeLayout, Size{80, 0}};
    Button homeReset{&homeLayout, Size{80, 0}};
  Label firmwareLabel{this, Size{~0, 0}, 5};
    HorizontalLayout firmwareLayout{this, Size{~0, 0}};
    LineEdit firmwarePath{&firmwareLayout, Size{~0, 0}};
    Button firmwareAssign{&firmwareLayout, Size{80, 0}};
    Button firmwareReset{&firmwareLayout, Size{80, 0}};
  Label savesLabel{this, Size{~0, 0}, 5};
  HorizontalLayout savesLayout{this, Size{~0, 0}};
    LineEdit savesPath{&savesLayout, Size{~0, 0}};
    Button savesAssign{&savesLayout, Size{80, 0}};
    Button savesReset{&savesLayout, Size{80, 0}};
  Label screenshotsLabel{this, Size{~0, 0}, 5};
  HorizontalLayout screenshotsLayout{this, Size{~0, 0}};
    LineEdit screenshotsPath{&screenshotsLayout, Size{~0, 0}};
    Button screenshotsAssign{&screenshotsLayout, Size{80, 0}};
    Button screenshotsReset{&screenshotsLayout, Size{80, 0}};
  Label debuggingLabel{this, Size{~0, 0}, 5};
  HorizontalLayout debuggingLayout{this, Size{~0, 0}};
    LineEdit debuggingPath{&debuggingLayout, Size{~0, 0}};
    Button debuggingAssign{&debuggingLayout, Size{80, 0}};
    Button debuggingReset{&debuggingLayout, Size{80, 0}};
  Label arcadeRomsLabel{this, Size{~0, 0}, 5};
  HorizontalLayout arcadeRomsLayout{this, Size{~0, 0}};
    LineEdit arcadeRomsPath{&arcadeRomsLayout, Size{~0, 0}};
    Button arcadeRomsAssign{&arcadeRomsLayout, Size{80, 0}};
    Button arcadeRomsReset{&arcadeRomsLayout, Size{80, 0}};
};

struct DriverSettings : VerticalLayout {
  auto construct() -> void;
  auto videoRefresh() -> void;
  auto videoDriverUpdate() -> void;
  auto audioRefresh() -> void;
  auto audioDriverUpdate() -> void;
  auto inputRefresh() -> void;
  auto inputDriverUpdate() -> void;

  Label videoLabel{this, Size{~0, 0}, 5};
  HorizontalLayout videoDriverLayout{this, Size{~0, 0}};
    Label videoDriverLabel{&videoDriverLayout, Size{0, 0}};
    ComboButton videoDriverList{&videoDriverLayout, Size{0, 0}};
    Button videoDriverAssign{&videoDriverLayout, Size{0, 0}};
    Label videoDriverActive{&videoDriverLayout, Size{0, 0}};
  HorizontalLayout videoPropertyLayout{this, Size{~0, 0}};
    Label videoMonitorLabel{&videoPropertyLayout, Size{0, 0}};
    ComboButton videoMonitorList{&videoPropertyLayout, Size{0, 0}};
    Label videoFormatLabel{&videoPropertyLayout, Size{0, 0}};
    ComboButton videoFormatList{&videoPropertyLayout, Size{0, 0}};
  HorizontalLayout videoToggleLayout{this, Size{~0, 0}};
    CheckLabel videoExclusiveToggle{&videoToggleLayout, Size{0, 0}};
    CheckLabel videoBlockingToggle{&videoToggleLayout, Size{0, 0}};
    CheckLabel videoFlushToggle{&videoToggleLayout, Size{0, 0}};
  //
  Label audioLabel{this, Size{~0, 0}, 5};
  HorizontalLayout audioDriverLayout{this, Size{~0, 0}};
    Label audioDriverLabel{&audioDriverLayout, Size{0, 0}};
    ComboButton audioDriverList{&audioDriverLayout, Size{0, 0}};
    Button audioDriverAssign{&audioDriverLayout, Size{0, 0}};
    Label audioDriverActive{&audioDriverLayout, Size{0, 0}};
  HorizontalLayout audioDeviceLayout{this, Size{~0, 0}};
    Label audioDeviceLabel{&audioDeviceLayout, Size{0, 0}};
    ComboButton audioDeviceList{&audioDeviceLayout, Size{0, 0}};
  HorizontalLayout audioPropertyLayout{this, Size{~0, 0}};
    Label audioFrequencyLabel{&audioPropertyLayout, Size{0, 0}};
    ComboButton audioFrequencyList{&audioPropertyLayout, Size{0, 0}};
    Label audioLatencyLabel{&audioPropertyLayout, Size{0, 0}};
    ComboButton audioLatencyList{&audioPropertyLayout, Size{0, 0}};
  HorizontalLayout audioToggleLayout{this, Size{~0, 0}};
    CheckLabel audioExclusiveToggle{&audioToggleLayout, Size{0, 0}};
    CheckLabel audioBlockingToggle{&audioToggleLayout, Size{0, 0}};
    CheckLabel audioDynamicToggle{&audioToggleLayout, Size{0, 0}};
  //
  Label inputLabel{this, Size{~0, 0}, 5};
  HorizontalLayout inputDriverLayout{this, Size{~0, 0}};
    Label inputDriverLabel{&inputDriverLayout, Size{0, 0}};
    ComboButton inputDriverList{&inputDriverLayout, Size{0, 0}};
    Button inputDriverAssign{&inputDriverLayout, Size{0, 0}};
    Label inputDriverActive{&inputDriverLayout, Size{0, 0}};
  HorizontalLayout inputDefocusLayout{this, Size{~0, 0}};
    Label inputDefocusLabel{&inputDefocusLayout, Size{0, 0}};
    RadioLabel inputDefocusPause{&inputDefocusLayout, Size{0, 0}};
    RadioLabel inputDefocusBlock{&inputDefocusLayout, Size{0, 0}};
    RadioLabel inputDefocusAllow{&inputDefocusLayout, Size{0, 0}};
    Group inputDefocusGroup{&inputDefocusPause, &inputDefocusBlock, &inputDefocusAllow};
};

struct DebugSettings : VerticalLayout {
  auto construct() -> void;
  auto infoRefresh() -> void;
  auto serverRefresh() -> void;

  Label debugLabel{this, Size{~0, 0}, 5};

  HorizontalLayout portLayout{this, Size{~0, 0}};
    Label portLabel{&portLayout, Size{48, 20}};
    LineEdit port{&portLayout, Size{~0, 0}};
    Label portHint{&portLayout, Size{~0, 0}};

  HorizontalLayout ipv4Layout{this, Size{~0, 0}};
    Label ipv4Label{&ipv4Layout, Size{48, 20}};
    CheckLabel ipv4{&ipv4Layout, Size{~0, 0}};

  HorizontalLayout enabledLayout{this, Size{~0, 0}};
    Label enabledLabel{&enabledLayout, Size{48, 20}};
    CheckLabel enabled{&enabledLayout, Size{~0, 0}};

  Label connectInfo{this, Size{~0, 30}, 5};
};

struct HomePanel : VerticalLayout {
  auto construct() -> void;

  Canvas canvas{this, Size{~0, ~0}};
};

struct SettingsWindow : Window {
  SettingsWindow();
  auto show(const string& panel) -> void;
  auto eventChange() -> void;

  HorizontalLayout layout{this};
    ListView panelList{&layout, Size{125_sx, ~0}};
    VerticalLayout panelContainer{&layout, Size{~0, ~0}};
      VideoSettings videoSettings;
      AudioSettings audioSettings;
      InputSettings inputSettings;
      HotkeySettings hotkeySettings;
      EmulatorSettings emulatorSettings;
      OptionSettings optionSettings;
      FirmwareSettings firmwareSettings;
      PathSettings pathSettings;
      DriverSettings driverSettings;
      DebugSettings debugSettings;
      HomePanel homePanel;
};

extern Settings settings;
namespace Instances { extern Instance<SettingsWindow> settingsWindow; }
extern SettingsWindow& settingsWindow;
extern VideoSettings& videoSettings;
extern AudioSettings& audioSettings;
extern InputSettings& inputSettings;
extern HotkeySettings& hotkeySettings;
extern EmulatorSettings& emulatorSettings;
extern OptionSettings& optionSettings;
extern FirmwareSettings& firmwareSettings;
extern PathSettings& pathSettings;
extern DriverSettings& driverSettings;
extern DebugSettings& debugSettings;