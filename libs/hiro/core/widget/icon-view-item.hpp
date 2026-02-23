#if defined(Hiro_IconView)
struct mIconViewItem : mObject {
  Declare(IconViewItem)

  auto icon() const -> multiFactorImage;
  auto remove() -> type& override;
  auto selected() const -> bool;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setSelected(bool selected = true) -> type&;
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
