#if defined(Hiro_TableView)

namespace hiro {

struct pTableView : pWidget {
  Declare(TableView, Widget)

  auto append(sTableViewColumn header) -> void;
  auto append(sTableViewItem item) -> void;
  auto remove(sTableViewColumn header) -> void;
  auto remove(sTableViewItem item) -> void;
  auto resizeColumns() -> void;
  auto setAlignment(Alignment alignment) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setBatchable(bool batchable) -> void;
  auto setBordered(bool bordered) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setHeadered(bool headered) -> void;
  auto setSortable(bool sortable) -> void;

  auto _onSize() -> void;
  auto _width(u32 column) -> u32;
  auto _widthOfColumn(u32 column) -> u32;
  auto _widthOfCell(u32 row, u32 column) -> u32;

  QtTableView* qtTableView = nullptr;
  QtTableViewDelegate* qtTableViewDelegate = nullptr;
};

}

#endif
