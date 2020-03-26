struct Settings {
  auto load() -> void;
  auto save() -> void;
  auto properties(bool mode, Markup::Node document) -> void;

  struct {
    string driver = Video::safestDriver();
    string monitor = "Primary";
    string format = "ARGB24";
    bool exclusive = false;
    bool blocking = false;
    bool flush = false;
    double luminance = 1.0;
    double saturation = 1.0;
    double gamma = 1.0;
    string output = "Center";
    bool aspectCorrection = true;
    uint scale = 2;
    string shader = "Blur";
  } video;

  struct {
    string driver = Audio::safestDriver();
    string device = "Default";
    uint frequency = 48000;
    uint latency = 0;
    bool exclusive = false;
    bool blocking = true;
    bool dynamic = false;
    int skew = 0;
    double volume = 1.0;
    double balance = 0.0;
    bool mute = false;
  } audio;

  struct {
    string driver = Input::safestDriver();
    string unfocused = "Block";
  } input;

  struct {
    bool showStatusBar = true;
    bool showSystemPanels = true;
    bool advancedMode = false;
  } interface;
};

extern Settings settings;
