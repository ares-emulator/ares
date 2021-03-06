#if defined(Hiro_Application)
struct Application {
  Application() = delete;

  static auto abort() -> void;
  static auto doMain() -> void;
  static auto doOpenFile(const string& path) -> void;
  static auto exit() -> void;
  static auto font() -> Font;
  static auto locale() -> Locale&;
  static auto modal() -> bool;
  static auto name() -> string;
  static auto onMain(const function<void ()>& callback = {}) -> void;
  static auto onOpenFile(const function<void (const string& path)>& callback = {}) -> void;
  static auto run() -> void;
  static auto scale() -> f32;
  static auto scale(f32 value) -> f32;
  static auto pendingEvents() -> bool;
  static auto processEvents() -> void;
  static auto quit() -> void;
  static auto screenSaver() -> bool;
  static auto setFont(const Font& font = {}) -> void;
  static auto setName(const string& name = "") -> void;
  static auto setScale(f32 scale = 1.0) -> void;
  static auto setScreenSaver(bool screenSaver = true) -> void;
  static auto setToolTips(bool toolTips = true) -> void;
  static auto toolTips() -> bool;
  static auto unscale(f32 value) -> f32;

  struct Cocoa {
    static auto doAbout() -> void;
    static auto doActivate() -> void;
    static auto doPreferences() -> void;
    static auto doQuit() -> void;
    static auto onAbout(const function<void ()>& callback = {}) -> void;
    static auto onActivate(const function<void ()>& callback = {}) -> void;
    static auto onPreferences(const function<void ()>& callback = {}) -> void;
    static auto onQuit(const function<void ()>& callback = {}) -> void;
  };

  struct Namespace : Locale::Namespace {
    Namespace(const string& value) : Locale::Namespace(Application::locale(), value) {}
  };

//private:
  struct State {
    Font font;
    bool initialized = false;
    Locale locale;
    s32 modal = 0;
    string name;
    function<void ()> onMain;
    function<void (const string& path)> onOpenFile;
    bool quit = false;
    f32 scale = 1.0;
    bool screenSaver = true;
    bool toolTips = true;

    struct Cocoa {
      function<void ()> onAbout;
      function<void ()> onActivate;
      function<void ()> onPreferences;
      function<void ()> onQuit;
    } cocoa;
  };

  static auto initialize() -> void;
  static auto state() -> State&;
};
#endif
