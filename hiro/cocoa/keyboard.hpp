#if defined(Hiro_Keyboard)

namespace hiro {

struct pKeyboard {
  static auto poll() -> std::vector<bool>;
  static auto pressed(u32 code) -> bool;
};

}

#endif
