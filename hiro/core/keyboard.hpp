#if defined(Hiro_Keyboard)
struct Keyboard {
  Keyboard() = delete;

  static auto append(Hotkey hotkey) -> void;
  static auto hotkey(u32 position) -> Hotkey;
  static auto hotkeyCount() -> u32;
  static auto hotkeys() -> std::vector<Hotkey>;
  static auto poll() -> std::vector<bool>;
  static auto pressed(const string& key) -> bool;
  static auto released(const string& key) -> bool;
  static auto remove(Hotkey hotkey) -> void;

  static const std::vector<string> keys;

//private:
  struct State {
    std::vector<Hotkey> hotkeys;
  };
  static State state;
};
#endif
