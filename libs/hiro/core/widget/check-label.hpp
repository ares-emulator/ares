#if defined(Hiro_CheckLabel)
struct mCheckLabel : mWidget {
  Declare(CheckLabel)

  auto checked() const -> bool;
  auto doToggle() const -> void;
  auto onToggle(const std::function<void ()>& callback = {}) -> type&;
  auto setChecked(bool checked = true) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    bool checked = false;
    std::function<void ()> onToggle;
    string text;
  } state;
};
#endif
