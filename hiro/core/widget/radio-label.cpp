#if defined(Hiro_RadioLabel)

auto mRadioLabel::allocate() -> pObject* {
  return new pRadioLabel(*this);
}

//

auto mRadioLabel::checked() const -> bool {
  return state.checked;
}

auto mRadioLabel::doActivate() const -> void {
  if(state.onActivate) return state.onActivate();
}

auto mRadioLabel::group() const -> Group {
  return state.group;
}

auto mRadioLabel::onActivate(const std::function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mRadioLabel::setChecked() -> type& {
  if(auto group = this->group()) {
    for(auto& weak : group->state.objects) {
      if(auto object = weak.lock()) {
        if(auto radioLabel = dynamic_cast<mRadioLabel*>(object.get())) {
          radioLabel->state.checked = false;
        }
      }
    }
  }
  state.checked = true;
  signal(setChecked);
  return *this;
}

auto mRadioLabel::setGroup(sGroup group) -> type& {
  state.group = group ? group : Group{&instance};
  signal(setGroup, group);
  return *this;
}

auto mRadioLabel::setText(const string& text) -> type& {
  state.text = text;
  signal(setText, text);
  return *this;
}

auto mRadioLabel::text() const -> string {
  return state.text;
}

#endif
