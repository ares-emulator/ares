#if defined(Hiro_Mouse)
struct Mouse {
  enum class Button : u32 { Left, Middle, Right };
  enum class Click : u32 { Single, Double };

  Mouse() = delete;

  static auto position() -> Position;
  static auto pressed(Button) -> bool;
  static auto released(Button) -> bool;
};
#endif
