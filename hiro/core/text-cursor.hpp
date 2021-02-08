#if defined(Hiro_TextCursor)
struct TextCursor {
  using type = TextCursor;

  TextCursor(s32 offset = 0, s32 length = 0);

  explicit operator bool() const;
  auto operator==(const TextCursor& source) const -> bool;
  auto operator!=(const TextCursor& source) const -> bool;

  auto length() const -> s32;
  auto offset() const -> s32;
  auto setLength(s32 length = 0) -> type&;
  auto setOffset(s32 offset = 0) -> type&;
  auto setTextCursor(s32 offset = 0, s32 length = 0) -> type&;

//private:
  struct State {
    s32 offset;
    s32 length;
  } state;
};
#endif
