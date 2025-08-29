#if defined(Hiro_MenuBar)

auto mMenuBar::allocate() -> pObject* {
  return new pMenuBar(*this);
}

auto mMenuBar::destruct() -> void {
  for(auto& menu : state.menus) menu->destruct();
  mObject::destruct();
}

//

auto mMenuBar::append(sMenu menu) -> type& {
  state.menus.push_back(menu);
  menu->setParent(this, menuCount() - 1);
  signal(append, menu);
  return *this;
}

auto mMenuBar::menu(u32 position) const -> Menu {
  if(position < menuCount()) return state.menus[position];
  return {};
}

auto mMenuBar::menuCount() const -> u32 {
  return state.menus.size();
}

auto mMenuBar::menus() const -> std::vector<Menu> {
  std::vector<Menu> menus;
  for(auto& menu : state.menus) menus.push_back(menu);
  return menus;
}

auto mMenuBar::remove() -> type& {
  if(auto window = parentWindow()) window->remove(window->menuBar());
  return *this;
}

auto mMenuBar::remove(sMenu menu) -> type& {
  s32 offset = menu->offset();
  signal(remove, menu);
  state.menus.erase(state.menus.begin() + offset);
  for(auto n : range(offset, menuCount())) {
    state.menus[n]->adjustOffset(-1);
  }
  menu->setParent();
  return *this;
}

auto mMenuBar::reset() -> type& {
  state.menus.clear();
  return *this;
}

auto mMenuBar::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& menu : state.menus | std::views::reverse) menu->destruct();
  mObject::setParent(parent, offset);
  for(auto& menu : state.menus) menu->setParent(this, menu->offset());
  return *this;
}

#endif
