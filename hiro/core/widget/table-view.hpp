#if defined(Hiro_TableView)
struct mTableView : mWidget {
  Declare(TableView)
  using mObject::remove;

  auto alignment() const -> Alignment;
  auto append(sTableViewColumn column) -> type&;
  auto append(sTableViewItem item) -> type&;
  auto backgroundColor() const -> Color;
  auto batchable() const -> bool;
  auto batched() const -> std::vector<TableViewItem>;
  auto bordered() const -> bool;
  auto column(u32 position) const -> TableViewColumn;
  auto columnCount() const -> u32;
  auto columns() const -> std::vector<TableViewColumn>;
  auto doActivate(sTableViewCell cell) const -> void;
  auto doChange() const -> void;
  auto doContext(sTableViewCell cell) const -> void;
  auto doEdit(sTableViewCell cell) const -> void;
  auto doSort(sTableViewColumn column) const -> void;
  auto doToggle(sTableViewCell cell) const -> void;
  auto foregroundColor() const -> Color;
  auto headered() const -> bool;
  auto item(u32 position) const -> TableViewItem;
  auto itemCount() const -> u32;
  auto items() const -> std::vector<TableViewItem>;
  auto onActivate(const function<void (TableViewCell)>& callback = {}) -> type&;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto onContext(const function<void (TableViewCell)>& callback = {}) -> type&;
  auto onEdit(const function<void (TableViewCell)>& callback = {}) -> type&;
  auto onSort(const function<void (TableViewColumn)>& callback = {}) -> type&;
  auto onToggle(const function<void (TableViewCell)>& callback = {}) -> type&;
  auto remove(sTableViewColumn column) -> type&;
  auto remove(sTableViewItem item) -> type&;
  auto reset() -> type& override;
  auto resizeColumns() -> type&;
  auto selectAll() -> type&;
  auto selectNone() -> type&;
  auto selected() const -> TableViewItem;
  auto setAlignment(Alignment alignment = {}) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setBatchable(bool batchable = true) -> type&;
  auto setBordered(bool bordered = true) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setHeadered(bool headered = true) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setSortable(bool sortable = true) -> type&;
  auto setUsesSidebarStyle(bool usesSidebarStyle = true) -> type&;
  auto sort() -> type&;
  auto sortable() const -> bool;

//private:
  struct State {
    u32 activeColumn = 0;
    Alignment alignment;
    Color backgroundColor;
    bool batchable = false;
    bool bordered = false;
    std::vector<sTableViewColumn> columns;
    Color foregroundColor;
    bool headered = false;
    std::vector<sTableViewItem> items;
    function<void (TableViewCell)> onActivate;
    function<void ()> onChange;
    function<void (TableViewCell)> onContext;
    function<void (TableViewCell)> onEdit;
    function<void (TableViewColumn)> onSort;
    function<void (TableViewCell)> onToggle;
    bool sortable = false;
  } state;

  auto destruct() -> void override;
};
#endif
