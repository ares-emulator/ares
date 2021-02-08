#if defined(Hiro_ComboEdit)
struct mComboEditItem : mObject {
  Declare(ComboEditItem)

  auto icon() const -> image;
  auto remove() -> type& override;
  auto setIcon(const image& icon = {}) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    image icon;
    string text;
  } state;
};
#endif
