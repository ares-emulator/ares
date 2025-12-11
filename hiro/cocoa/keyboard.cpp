#if defined(Hiro_Keyboard)

namespace hiro {

auto pKeyboard::poll() -> std::vector<bool> {
  std::vector<bool> result;
  return result;
}

auto pKeyboard::pressed(u32 code) -> bool {
  return false;
}

}

#endif
