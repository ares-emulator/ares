#if defined(Hiro_Button)
struct mButton : mWidget {
  Declare(Button)

  auto bordered() const -> bool;
  auto doActivate() const -> void;
  auto icon() const -> multiFactorImage;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto orientation() const -> Orientation;
  auto setBordered(bool bordered = true) -> type&;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setOrientation(Orientation orientation = Orientation::Horizontal) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    bool bordered = true;
    multiFactorImage icon;
    function<void ()> onActivate;
    Orientation orientation = Orientation::Horizontal;
    string text;
  } state;
};
#endif
