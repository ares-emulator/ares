#if defined(Hiro_HexEdit)

namespace hiro {

struct pHexEdit : pWidget {
  Declare(HexEdit, Widget)

  auto setAddress(u32 address) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setColumns(u32 columns) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setLength(u32 length) -> void;
  auto setRows(u32 rows) -> void;
  auto update() -> void;

  auto _keyPressEvent(QKeyEvent*) -> void;
  auto _rows() -> s32;
  auto _rowsScrollable() -> s32;
  auto _scrollTo(s32 position) -> void;
  auto _setState() -> void;

  QtHexEdit* qtHexEdit = nullptr;
  QHBoxLayout* qtLayout = nullptr;
  QtHexEditScrollBar* qtScrollBar = nullptr;
};

}

#endif
