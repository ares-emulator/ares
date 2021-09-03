#if defined(Hiro_MenuItem)
struct mMenuItem : mAction {
  Declare(MenuItem)

  auto doActivate() const -> void;
  auto icon() const -> multiFactorImage;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto setIcon(const multiFactorImage& icon = {}, bool force = false) -> type&;
  auto setIconForFile(const string& filename) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    multiFactorImage icon;
    function<void ()> onActivate;
    string text;
  } state;
};
#endif
