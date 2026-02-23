#if defined(Hiro_ComboButton)
struct mComboButtonItem : mObject {
  Declare(ComboButtonItem)

  auto icon() const -> multiFactorImage;
  auto remove() -> type& override;
  auto selected() const -> bool;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setSelected() -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    multiFactorImage icon;
    bool selected = false;
    string text;
  } state;
};
#endif
