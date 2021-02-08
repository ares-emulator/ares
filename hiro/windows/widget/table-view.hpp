#if defined(Hiro_TableView)

namespace hiro {

struct pTableView : pWidget {
  Declare(TableView, Widget)

  auto append(sTableViewColumn column) -> void;
  auto append(sTableViewItem item) -> void;
  auto remove(sTableViewColumn column) -> void;
  auto remove(sTableViewItem item) -> void;
  auto resizeColumns() -> void;
  auto setAlignment(Alignment alignment) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setBatchable(bool batchable) -> void;
  auto setBordered(bool bordered) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setGeometry(Geometry geometry) -> void override;
  auto setHeadered(bool headered) -> void;
  auto setSortable(bool sortable) -> void;

  auto onActivate(LPARAM lparam) -> void;
  auto onChange(LPARAM lparam) -> void;
  auto onContext(LPARAM lparam) -> void;
  auto onCustomDraw(LPARAM lparam) -> LRESULT;
  auto onSort(LPARAM lparam) -> void;
  auto onToggle(LPARAM lparam) -> void;
  auto windowProc(HWND, UINT, WPARAM, LPARAM) -> maybe<LRESULT> override;

  auto _backgroundColor(u32 row, u32 column) -> Color;
  auto _cellWidth(u32 row, u32 column) -> u32;
  auto _columnWidth(u32 column) -> u32;
  auto _font(u32 row, u32 column) -> Font;
  auto _foregroundColor(u32 row, u32 column) -> Color;
  auto _setIcons() -> void;
  auto _width(u32 column) -> u32;

  TableViewCell activateCell;
  HIMAGELIST imageList = 0;
  vector<image> icons;
};

}

#endif
