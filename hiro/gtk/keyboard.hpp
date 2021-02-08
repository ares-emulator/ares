#if defined(Hiro_Keyboard)

namespace hiro {

struct pKeyboard {
  static auto poll() -> vector<bool>;
  static auto pressed(u32 code) -> bool;

  static auto _pressed(const char* state, u16 code) -> bool;
  static auto _translate(u32 code) -> s32;

  static auto initialize() -> void;
};

}

#endif
