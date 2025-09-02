#if defined(Hiro_PopupMenu)

auto mPopupMenu::allocate() -> pObject* {
  return new pPopupMenu(*this);
}

auto mPopupMenu::destruct() -> void {
  for(auto& action : state.actions) action->destruct();
  mObject::destruct();
}

//

auto mPopupMenu::action(u32 position) const -> Action {
  if(position < actionCount()) return state.actions[position];
  return {};
}

auto mPopupMenu::actionCount() const -> u32 {
  return state.actions.size();
}

auto mPopupMenu::actions() const -> std::vector<Action> {
  std::vector<Action> actions;
  for(auto& action : state.actions) actions.push_back(action);
  return actions;
}

auto mPopupMenu::append(sAction action) -> type& {
  state.actions.push_back(action);
  action->setParent(this, actionCount() - 1);
  signal(append, action);
  return *this;
}

auto mPopupMenu::remove(sAction action) -> type& {
  s32 offset = action->offset();
  signal(remove, action);
  state.actions.erase(state.actions.begin() + offset);
  for(auto n : range(offset, actionCount())) {
    state.actions[n]->adjustOffset(-1);
  }
  action->setParent();
  return *this;
}

auto mPopupMenu::reset() -> type& {
  while(!state.actions.empty()) remove(state.actions.back());
  return *this;
}

auto mPopupMenu::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& action : state.actions | std::views::reverse) action->destruct();
  mObject::setParent(parent, offset);
  for(auto& action : state.actions) action->construct();
  return *this;
}

auto mPopupMenu::setVisible(bool visible) -> type& {
  signal(setVisible, visible);
  return *this;
}

#endif
