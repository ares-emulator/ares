#if defined(Hiro_Hotkey)
struct Hotkey {
  using type = Hotkey;

  Hotkey();
  Hotkey(const string& sequence);

  explicit operator bool() const;
  auto operator==(const Hotkey& source) const -> bool;
  auto operator!=(const Hotkey& source) const -> bool;

  auto doPress() const -> void;
  auto doRelease() const -> void;
  auto onPress(const std::function<void ()>& callback = {}) -> type&;
  auto onRelease(const std::function<void ()>& callback = {}) -> type&;
  auto reset() -> type&;
  auto sequence() const -> string;
  auto setSequence(const string& sequence = "") -> type&;

//private:
  struct State {
    bool active = false;
    std::vector<u32> keys;
    std::function<void ()> onPress;
    std::function<void ()> onRelease;
    string sequence;
  };
  shared_pointer<State> state;
};
#endif
