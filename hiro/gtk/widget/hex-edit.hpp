#if defined(Hiro_HexEdit)

namespace hiro {

struct pHexEdit : pWidget {
  Declare(HexEdit, Widget)

  auto focused() const -> bool override;
  auto setAddress(uint address) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setColumns(uint columns) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setLength(uint length) -> void;
  auto setRows(uint rows) -> void;
  auto update() -> void;

  auto cursorPosition() -> uint;
  auto keyPress(uint scancode, uint mask) -> bool;
  auto rows() -> int;
  auto rowsScrollable() -> int;
  auto scroll(int position) -> void;
  auto setCursorPosition(uint position) -> void;
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
