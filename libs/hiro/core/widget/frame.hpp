#if defined(Hiro_Frame)
struct mFrame : mWidget {
  Declare(Frame)
  using mObject::remove;

  auto append(sSizable sizable) -> type&;
  auto remove(sSizable sizable) -> type&;
  auto reset() -> type& override;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setText(const string& text = "") -> type&;
  auto sizable() const -> Sizable;
  auto text() const -> string;

//private:
  struct State {
    sSizable sizable;
    string text;
  } state;

  auto destruct() -> void override;
};
#endif
