#if defined(Hiro_MenuItem)
// LIJI: TODO: is there a more reasonable place to have this include?
#include "../../resource/icon.hpp"

auto mMenuItem::allocate() -> pObject* {
  return new pMenuItem(*this);
}

//

auto mMenuItem::doActivate() const -> void {
  if(state.onActivate) return state.onActivate();
}

auto mMenuItem::icon() const -> multiFactorImage {
  return state.icon;
}

auto mMenuItem::onActivate(const function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mMenuItem::setIcon(const multiFactorImage& icon, bool force) -> type& {
  state.icon = icon;
  signal(setIcon, icon, force);
  return *this;
}

auto mMenuItem::setIconForFile(const string& filename) -> type& {
    #if defined(PLATFORM_MACOS)
    state.icon = {};
    signal(setIconForFile, filename);
    #else
    if(directory::exists(filename)) setIcon(Icon::Emblem::Folder, true);
    else setIcon(Icon::Emblem::File, true);
    #endif
    return *this;
}

auto mMenuItem::setText(const string& text) -> type& {
  state.text = text;
  signal(setText, text);
  return *this;
}

auto mMenuItem::text() const -> string {
  return state.text;
}

#endif
