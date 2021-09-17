#if defined(Hiro_LineEdit)
struct mLineEdit : mWidget {
  Declare(LineEdit)

  auto backgroundColor() const -> Color;
  auto doActivate() const -> void;
  auto doChange() const -> void;
  auto editable() const -> bool;
  auto foregroundColor() const -> Color;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setEditable(bool editable = true) -> type&;
  auto setForegroundColor(SystemColor color) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    Color backgroundColor;
    bool editable = true;
    Color foregroundColor;
    function<void ()> onActivate;
    function<void ()> onChange;
    string text;
  } state;
};
#endif
