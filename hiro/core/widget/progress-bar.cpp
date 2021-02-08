#if defined(Hiro_ProgressBar)

auto mProgressBar::allocate() -> pObject* {
  return new pProgressBar(*this);
}

//

auto mProgressBar::position() const -> u32 {
  return state.position;
}

auto mProgressBar::setPosition(u32 position) -> type& {
  state.position = position;
  signal(setPosition, position);
  return *this;
}

#endif
