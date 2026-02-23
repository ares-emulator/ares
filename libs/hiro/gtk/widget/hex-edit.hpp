#if defined(Hiro_HexEdit)

namespace hiro {

struct pHexEdit : pWidget {
  Declare(HexEdit, Widget)

  auto focused() const -> bool override;
  auto setAddress(u32 address) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setColumns(u32 columns) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setLength(u32 length) -> void;
  auto setRows(u32 rows) -> void;
  auto update() -> void;

  auto cursorPosition() -> u32;
  auto keyPress(u32 scancode, u32 mask) -> bool;
  auto rows() -> s32;
  auto rowsScrollable() -> s32;
  auto scroll(s32 position) -> void;
  auto setCursorPosition(u32 position) -> void;
  auto setScroll() -> void;
  auto updateScroll() -> void;

  GtkWidget* container = nullptr;
  GtkWidget* subWidget = nullptr;
  GtkWidget* scrollBar = nullptr;
  GtkTextBuffer* textBuffer = nullptr;
  GtkTextMark* textCursor = nullptr;
};

}

#endif
