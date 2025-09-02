#if defined(Hiro_TableView)
struct mTableViewItem : mObject {
  Declare(TableViewItem)

  auto alignment() const -> Alignment;
  auto append(sTableViewCell cell) -> type&;
  auto backgroundColor() const -> Color;
  auto cell(u32 position) const -> TableViewCell;
  auto cellCount() const -> u32;
  auto cells() const -> std::vector<TableViewCell>;
  auto foregroundColor() const -> Color;
  auto remove() -> type& override;
  auto remove(sTableViewCell cell) -> type&;
  auto reset() -> type& override;
  auto selected() const -> bool;
  auto setAlignment(Alignment alignment = {}) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setFocused() -> type& override;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setSelected(bool selected = true) -> type&;

//private:
  struct State {
    Alignment alignment;
    Color backgroundColor;
    std::vector<sTableViewCell> cells;
    Color foregroundColor;
    bool selected = false;
  } state;

  auto destruct() -> void override;
};
#endif
