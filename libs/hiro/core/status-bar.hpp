#if defined(Hiro_StatusBar)
struct mStatusBar : mObject {
  Declare(StatusBar)

  auto remove() -> type& override;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    string text;
  } state;
};
#endif
