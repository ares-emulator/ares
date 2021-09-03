#if defined(Hiro_ComboEdit)
struct mComboEditItem : mObject {
  Declare(ComboEditItem)

  auto icon() const -> multiFactorImage;
  auto remove() -> type& override;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    multiFactorImage icon;
    string text;
  } state;
};
#endif
