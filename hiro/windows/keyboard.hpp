#if defined(Hiro_Keyboard)

namespace hiro {

struct pKeyboard {
  static auto poll() -> vector<bool>;
  static auto pressed(u32 code) -> bool;

  static auto initialize() -> void;

  static auto _translate(u32 code, u32 flags) -> signed;

  static vector<u16> keycodes;
};

}

#endif
