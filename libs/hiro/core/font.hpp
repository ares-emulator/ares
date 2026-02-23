#if defined(Hiro_Font)
struct Font {
  using type = Font;

  Font(const string& family = "", f32 size = 0.0);

  explicit operator bool() const;
  auto operator==(const Font& source) const -> bool;
  auto operator!=(const Font& source) const -> bool;

  auto bold() const -> bool;
  auto family() const -> string;
  auto italic() const -> bool;
  auto reset() -> type&;
  auto setBold(bool bold = true) -> type&;
  auto setFamily(const string& family = "") -> type&;
  auto setItalic(bool italic = true) -> type&;
  auto setSize(f32 size = 0.0) -> type&;
  auto size() const -> f32;
  auto size(const string& text) const -> Size;

  static const string Sans;
  static const string Serif;
  static const string Mono;

  struct State {
    string family;
    f32 size = 0.0;
    char bold = false;
    char italic = false;
  } state;
};
#endif
