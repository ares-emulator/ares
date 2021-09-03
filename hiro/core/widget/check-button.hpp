#if defined(Hiro_CheckButton)
struct mCheckButton : mWidget {
  Declare(CheckButton)

  auto bordered() const -> bool;
  auto checked() const -> bool;
  auto doToggle() const -> void;
  auto icon() const -> multiFactorImage;
  auto onToggle(const function<void ()>& callback = {}) -> type&;
  auto orientation() const -> Orientation;
  auto setBordered(bool bordered = true) -> type&;
  auto setChecked(bool checked = true) -> type&;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setOrientation(Orientation orientation = Orientation::Horizontal) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    bool bordered = true;
    bool checked = false;
    multiFactorImage icon;
    function<void ()> onToggle;
    Orientation orientation = Orientation::Horizontal;
    string text;
  } state;
};
#endif
