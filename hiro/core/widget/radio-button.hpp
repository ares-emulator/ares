#if defined(Hiro_RadioButton)
struct mRadioButton : mWidget {
  Declare(RadioButton)

  auto bordered() const -> bool;
  auto checked() const -> bool;
  auto doActivate() const -> void;
  auto group() const -> Group override;
  auto icon() const -> multiFactorImage;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto orientation() const -> Orientation;
  auto setBordered(bool bordered = true) -> type&;
  auto setChecked() -> type&;
  auto setGroup(sGroup group = {}) -> type& override;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setOrientation(Orientation orientation = Orientation::Horizontal) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    bool bordered = true;
    bool checked = false;
    sGroup group;
    multiFactorImage icon;
    function<void ()> onActivate;
    Orientation orientation = Orientation::Horizontal;
    string text;
  } state;
};
#endif
