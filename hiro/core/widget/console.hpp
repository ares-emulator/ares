#if defined(Hiro_Console)
struct mConsole : mWidget {
  Declare(Console)

  auto backgroundColor() const -> Color;
  auto doActivate(string) const -> void;
  auto foregroundColor() const -> Color;
  auto onActivate(const function<void (string)>& callback = {}) -> type&;
  auto print(const string& text) -> type&;
  auto prompt() const -> string;
  auto reset() -> type& override;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setPrompt(const string& prompt = "") -> type&;

//private:
  struct State {
    Color backgroundColor;
    Color foregroundColor;
    function<void (string)> onActivate;
    string prompt;
  } state;
};
#endif
