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

  auto keyPress(u32 key) -> bool;
  auto rows() -> s32;
  auto rowsScrollable() -> s32;
  auto scrollPosition() -> s32;
  auto scrollTo(s32 position) -> void;
  auto windowProc(HWND, UINT, WPARAM, LPARAM) -> maybe<LRESULT> override;

  HWND scrollBar = nullptr;
  HBRUSH backgroundBrush = nullptr;
};

}

#endif
