#if defined(Hiro_TableLayout)

struct TableLayout;
struct TableLayoutColumn;
struct TableLayoutRow;
struct TableLayoutCell;

struct mTableLayout;
struct mTableLayoutColumn;
struct mTableLayoutRow;
struct mTableLayoutCell;

using sTableLayout = shared_pointer<mTableLayout>;
using sTableLayoutColumn = shared_pointer<mTableLayoutColumn>;
using sTableLayoutRow = shared_pointer<mTableLayoutRow>;
using sTableLayoutCell = shared_pointer<mTableLayoutCell>;

struct mTableLayout : mSizable {
  using type = mTableLayout;
  using mSizable::remove;

  auto alignment() const -> Alignment;
  auto append(sSizable sizable, Size size) -> type&;
  auto cell(u32 position) const -> TableLayoutCell;
  auto cell(u32 x, u32 y) const -> TableLayoutCell;
  auto cell(sSizable sizable) const -> TableLayoutCell;
  auto cells() const -> vector<TableLayoutCell>;
  auto cellCount() const -> u32;
  auto column(u32 position) const -> TableLayoutColumn;
  auto columns() const -> vector<TableLayoutColumn>;
  auto columnCount() const -> u32;
  auto minimumSize() const -> Size override;
  auto padding() const -> Geometry;
  auto remove(sSizable sizable) -> type&;
  auto remove(sTableLayoutCell cell) -> type&;
  auto reset() -> type& override;
  auto resize() -> type&;
  auto row(u32 position) const -> TableLayoutRow;
  auto rows() const -> vector<TableLayoutRow>;
  auto rowCount() const -> u32;
  auto setAlignment(Alignment alignment) -> type&;
  auto setEnabled(bool enabled) -> type& override;
  auto setFont(const Font& font) -> type& override;
  auto setGeometry(Geometry geometry) -> type& override;
  auto setPadding(Geometry padding) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setSize(Size size) -> type&;
  auto setVisible(bool visible) -> type& override;
  auto size() const -> Size;
  auto synchronize() -> type&;

private:
  auto destruct() -> void override;

  struct State {
    Alignment alignment;
    vector<TableLayoutCell> cells;
    vector<TableLayoutColumn> columns;
    Geometry padding;
    vector<TableLayoutRow> rows;
    Size size;
  } state;
};

struct mTableLayoutColumn : mObject {
  using type = mTableLayoutColumn;

  auto alignment() const -> Alignment;
  auto setAlignment(Alignment alignment) -> type&;
  auto setSpacing(f32 spacing) -> type&;
  auto spacing() const -> f32;
  auto synchronize() -> type&;

private:
  struct State {
    Alignment alignment;
    f32 spacing = 5_sx;
  } state;

  friend class mTableLayout;
};

struct mTableLayoutRow : mObject {
  using type = mTableLayoutRow;

  auto alignment() const -> Alignment;
  auto setAlignment(Alignment alignment) -> type&;
  auto setSpacing(f32 spacing) -> type&;
  auto spacing() const -> f32;
  auto synchronize() -> type&;

private:
  struct State {
    Alignment alignment;
    f32 spacing = 5_sy;
  } state;

  friend class mTableLayout;
};

struct mTableLayoutCell : mObject {
  using type = mTableLayoutCell;

  auto alignment() const -> Alignment;
  auto setAlignment(Alignment alignment) -> type&;
  auto setEnabled(bool enabled) -> type& override;
  auto setFont(const Font& font) -> type& override;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setSizable(sSizable sizable) -> type&;
  auto setSize(Size size) -> type&;
  auto setVisible(bool visible) -> type& override;
  auto sizable() const -> Sizable;
  auto size() const -> Size;
  auto synchronize() -> type&;

private:
  auto destruct() -> void override;

  struct State {
    Alignment alignment;
    sSizable sizable;
    Size size;
  } state;

  friend class mTableLayout;
};

#endif
