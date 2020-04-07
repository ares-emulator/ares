#if defined(Hiro_Keyboard)

namespace hiro {

struct pKeyboard {
  static auto poll() -> vector<bool>;
  static auto pressed(uint code) -> bool;

  static auto _pressed(const char* state, uint16_t code) -> bool;
  static auto _translate(uint code) -> signed;

  static auto initialize() -> void;
};

}

#endif
