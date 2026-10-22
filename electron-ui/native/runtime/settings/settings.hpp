// Shared by firmware discovery and the existing loader's BML bindings.
inline auto firmwareSettingPath(const string& system, const string& type, const string& region) -> string {
  string base = string{system}.replace(" ", "").replace("(", "").replace(")", "");
  return string{base, "/Firmware/", type, ".", region}.replace(" ", "-");
}

struct Settings : Markup::Node {
  using string = nall::string;

  auto process(bool load) -> void;

  string filePath;

  struct Video {
    string driver;
    string monitor;
    string format;
    bool exclusive = false;
    bool blocking = false;
    bool forceSRGB = false;
    bool threadedRenderer = true;
    bool nativeFullScreen = false;
    bool flush = false;
    string shader = "None";
    u32 windowWidth = 800;
    u32 windowHeight = 576;
    string output = "Scale";
    u32 fixedScale = 2;
    string aspectCorrection = "Standard";
    bool adaptiveSizing = true;
    bool autoCentering = false;

    f64 luminance = 1.0;
    f64 saturation = 1.0;
    f64 gamma = 1.0;
    bool colorBleed = false;
    bool colorEmulation = true;
    bool interframeBlending = true;
    bool overscan = true;
    bool pixelAccuracy = false;
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

    f64 volume = 0.4;
    f64 balance = 0.0;
  } audio;

  struct Input {
    string driver;
    string defocus = "Pause";
    string digitalToAnalog = "Immediate";
    u32 digitalToAnalogTime = 500;
  } input;

  struct Boot {
    bool fast = true;
    bool debugger = false;
    bool awaitGDBClient = false;
    string prefer = "NTSC-U";
  } boot;

  struct General {
    bool showStatusBar = true;
    bool rewind = false;
    bool runAhead = false;
    bool autoSaveMemory = true;
    bool noFilePrompt = false;
  } general;

  struct Rewind {
    u32 length = 80;
    u32 frequency = 60;
    bool mute = false;
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

  struct Developer {
    u32  debugServerPort = 9123;
    bool debugServerEnabled = false; // if enabled, server starts with ares
    bool debugServerUseIPv4 = false; // forces IPv4 over IPv6
    bool homebrewMode = false;
    bool deterministicEntropy = false;
    bool forceInterpreter = false;
  } developer;

  struct Nintendo64 {
    bool expansionPak = true;
    u8 controllerPakBankCount = 1;
    string controllerPakBankString = "32KiB (Default)";
    string quality = "SD";
    bool supersampling = false;
    bool disableVideoInterfaceProcessing = false;
    bool weaveDeinterlacing = true;
  } nintendo64;

  struct GameBoyAdvance {
    bool player = false;
  } gameBoyAdvance;

  struct SuperFamicom {
    bool deepBlackBoost = false;
  } superFamicom;

  struct MegaDrive {
    bool tmss = false;
  } megadrive;
};

extern Settings settings;
