#if defined(Hiro_MenuItem)
struct mMenuItem : mAction {
  Declare(MenuItem)

  auto doActivate() const -> void;
  auto icon() const -> image;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto setIcon(const image& icon = {}) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    image icon;
    function<void ()> onActivate;
    string text;
  } state;
};
#endif
