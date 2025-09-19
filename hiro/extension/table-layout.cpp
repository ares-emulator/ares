#if defined(Hiro_TableLayout)

auto mTableLayout::alignment() const -> Alignment {
  return state.alignment;
}

auto mTableLayout::append(sSizable sizable, Size size) -> type& {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return *this;
  }

  TableLayoutCell cell;
  cell->setSizable(sizable);
  cell->setSize(size);
  cell->setParent(this, cellCount());
  state.cells.push_back(cell);
  return *this;
}

auto mTableLayout::cell(u32 position) const -> TableLayoutCell {
  if(position < state.cells.size()) return state.cells[position];
  return {};
}

auto mTableLayout::cell(u32 x, u32 y) const -> TableLayoutCell {
  auto index = y * columnCount() + x;
  if(index < state.cells.size()) return state.cells[index];
  return {};
}

auto mTableLayout::cell(sSizable sizable) const -> TableLayoutCell {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return cell;
  }
  return {};
}

auto mTableLayout::cells() const -> std::vector<TableLayoutCell> {
  return state.cells;
}

auto mTableLayout::cellCount() const -> u32 {
  return state.cells.size();
}

auto mTableLayout::column(u32 position) const -> TableLayoutColumn {
  if(position < state.columns.size()) return state.columns[position];
  return {};
}

auto mTableLayout::columns() const -> std::vector<TableLayoutColumn> {
  return state.columns;
}

auto mTableLayout::columnCount() const -> u32 {
  return state.columns.size();
}

auto mTableLayout::destruct() -> void {
  for(auto& cell : state.cells) cell->destruct();
  for(auto& column : state.columns) column->destruct();
  for(auto& row : state.rows) row->destruct();
  mSizable::destruct();
}

auto mTableLayout::minimumSize() const -> Size {
  f32 minimumWidth = 0;
  for(u32 x : range(columnCount())) {
    f32 width = 0;
    auto column = this->column(x);
    for(u32 y : range(rowCount())) {
      auto row = this->row(y);
      auto cell = this->cell(x, y);
      if(cell.size().width() == Size::Minimum || cell.size().width() == Size::Maximum) {
        width = max(width, cell.sizable()->minimumSize().width());
      } else {
        width = max(width, cell.size().width());
      }
    }
    minimumWidth += width;
    if(x != columnCount() - 1) minimumWidth += column.spacing();
  }

  f32 minimumHeight = 0;
  for(u32 y : range(rowCount())) {
    f32 height = 0;
    auto row = this->row(y);
    for(u32 x : range(columnCount())) {
      auto column = this->column(x);
      auto cell = this->cell(x, y);
      if(cell.size().height() == Size::Minimum || cell.size().height() == Size::Maximum) {
        height = max(height, cell.sizable()->minimumSize().height());
      } else {
        height = max(height, cell.size().height());
      }
    }
    minimumHeight += height;
    if(y != rowCount() - 1) minimumHeight += row.spacing();
  }

  return {
    padding().x() + minimumWidth  + padding().width(),
    padding().y() + minimumHeight + padding().height()
  };
}

auto mTableLayout::padding() const -> Geometry {
  return state.padding;
}

auto mTableLayout::remove(sSizable sizable) -> type& {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return remove(cell);
  }
  return *this;
}

auto mTableLayout::remove(sTableLayoutCell cell) -> type& {
  if(cell->parent() != this) return *this;
  auto offset = cell->offset();
  cell->setParent();
  state.cells.erase(state.cells.begin() + offset);
  for(u32 n : range(offset, cellCount())) state.cells[n]->adjustOffset(-1);
  return synchronize();
}

auto mTableLayout::reset() -> type& {
  while(!state.cells.empty()) remove(state.cells.back());
  return synchronize();
}

auto mTableLayout::resize() -> type& {
  setGeometry(geometry());
  return *this;
}

auto mTableLayout::row(u32 position) const -> TableLayoutRow {
  if(position < state.rows.size()) return state.rows[position];
  return {};
}

auto mTableLayout::rows() const -> std::vector<TableLayoutRow> {
  return state.rows;
}

auto mTableLayout::rowCount() const -> u32 {
  return state.rows.size();
}

auto mTableLayout::setAlignment(Alignment alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mTableLayout::setEnabled(bool enabled) -> type& {
  mSizable::setEnabled(enabled);
  for(auto& cell : state.cells) cell.setEnabled(cell.enabled());
  return *this;
}

auto mTableLayout::setFont(const Font& font) -> type& {
  mSizable::setFont(font);
  for(auto& cell : state.cells) cell.setFont(cell.font());
  return *this;
}

auto mTableLayout::setGeometry(Geometry requestedGeometry) -> type& {
//if(!visible(true)) return mSizable::setGeometry(requestedGeometry), *this;

  auto geometry = requestedGeometry;
  geometry.setX(geometry.x() + padding().x());
  geometry.setY(geometry.y() + padding().y());
  geometry.setWidth (geometry.width()  - padding().x() - padding().width());
  geometry.setHeight(geometry.height() - padding().y() - padding().height());

  std::vector<f32> widths;
  widths.resize(columnCount());
  u32 maximumWidths = 0;
  for(u32 x : range(columnCount())) {
    f32 width = 0;
    auto column = this->column(x);
    for(u32 y : range(rowCount())) {
      auto row = this->row(y);
      auto cell = this->cell(x, y);
      if(cell.size().width() == Size::Maximum) {
        width = Size::Maximum;
        maximumWidths++;
        break;
      }
      if(cell.size().width() == Size::Minimum) {
        width = max(width, cell.sizable()->minimumSize().width());
      } else {
        width = max(width, cell.size().width());
      }
    }
    widths[x] = width;
  }

  std::vector<f32> heights;
  heights.resize(rowCount());
  u32 maximumHeights = 0;
  for(u32 y : range(rowCount())) {
    f32 height = 0;
    auto row = this->row(y);
    for(u32 x : range(columnCount())) {
      auto column = this->column(x);
      auto cell = this->cell(x, y);
      if(cell.size().height() == Size::Maximum) {
        height = Size::Maximum;
        maximumHeights++;
        break;
      }
      if(cell.size().height() == Size::Minimum) {
        height = max(height, cell.sizable()->minimumSize().height());
      } else {
        height = max(height, cell.size().height());
      }
    }
    heights[y] = height;
  }

  f32 fixedWidth = 0;
  for(u32 x : range(columnCount())) {
    if(widths[x] != Size::Maximum) fixedWidth += widths[x];
    if(x != columnCount() - 1) fixedWidth += column(x)->spacing();
  }
  f32 maximumWidth = (geometry.width() - fixedWidth) / maximumWidths;
  for(auto& width : widths) {
    if(width == Size::Maximum) width = maximumWidth;
  }

  f32 fixedHeight = 0;
  for(u32 y : range(rowCount())) {
    if(heights[y] != Size::Maximum) fixedHeight += heights[y];
    if(y != rowCount() - 1) fixedHeight += row(y)->spacing();
  }
  f32 maximumHeight = (geometry.height() - fixedHeight) / maximumHeights;
  for(auto& height : heights) {
    if(height == Size::Maximum) height = maximumHeight;
  }

  f32 geometryY = geometry.y();
  for(u32 y : range(rowCount())) {
    f32 geometryX = geometry.x();
    auto row = this->row(y);
    for(u32 x : range(columnCount())) {
      auto column = this->column(x);
      auto cell = this->cell(x, y);
      f32 geometryWidth  = widths [x];
      f32 geometryHeight = heights[y];
      auto alignment = cell.alignment();
      if(!alignment) alignment = column.alignment();
      if(!alignment) alignment = row.alignment();
      if(!alignment) alignment = this->alignment();
      if(!alignment) alignment = {0.0, 0.5};

      f32 cellWidth = cell.size().width();
      if(cellWidth == Size::Minimum) cellWidth = cell.sizable()->minimumSize().width();
      if(cellWidth == Size::Maximum) cellWidth = geometryWidth;
      cellWidth = min(cellWidth, geometryWidth);
      f32 cellHeight = cell.size().height();
      if(cellHeight == Size::Minimum) cellHeight = cell.sizable()->minimumSize().height();
      if(cellHeight == Size::Maximum) cellHeight = geometryHeight;
      cellHeight = min(cellHeight, geometryHeight);
      f32 cellX = geometryX + alignment.horizontal() * (geometryWidth  - cellWidth);
      f32 cellY = geometryY + alignment.vertical()   * (geometryHeight - cellHeight);
      cell.sizable()->setGeometry({cellX, cellY, cellWidth, cellHeight});

      geometryX += widths[x] + column.spacing();
    }
    geometryY += heights[y] + row.spacing();
  }

  mSizable::setGeometry(requestedGeometry);
  return *this;
}

auto mTableLayout::setPadding(Geometry padding) -> type& {
  state.padding = padding;
  return synchronize();
}

auto mTableLayout::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& cell : state.cells | std::views::reverse) cell->destruct();
  for(auto& column : state.columns | std::views::reverse) column->destruct();
  for(auto& row : state.rows | std::views::reverse) row->destruct();
  mObject::setParent(parent, offset);
  for(auto& cell : state.cells) cell->setParent(this, cell->offset());
  for(auto& column : state.columns) column->setParent(this, column->offset());
  for(auto& row : state.rows) row->setParent(this, row->offset());
  return *this;
}

auto mTableLayout::setSize(Size size) -> type& {
  state.size = size;
  state.columns.clear();
  state.rows.clear();
  for(auto x : range(size.width())) state.columns.push_back(TableLayoutColumn());
  for(auto y : range(size.height())) state.rows.push_back(TableLayoutRow());
  return synchronize();
}

auto mTableLayout::setVisible(bool visible) -> type& {
  mSizable::setVisible(visible);
  for(auto& cell : state.cells) cell.setVisible(cell.visible());
  return synchronize();
}

auto mTableLayout::size() const -> Size {
  return state.size;
}

auto mTableLayout::synchronize() -> type& {
  setGeometry(geometry());
  return *this;
}

//

auto mTableLayoutColumn::alignment() const -> Alignment {
  return state.alignment;
}

auto mTableLayoutColumn::setAlignment(Alignment alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mTableLayoutColumn::setSpacing(f32 spacing) -> type& {
  state.spacing = spacing;
  return synchronize();
}

auto mTableLayoutColumn::spacing() const -> f32 {
  return state.spacing;
}

auto mTableLayoutColumn::synchronize() -> type& {
  if(auto parent = this->parent()) {
    if(auto tableLayout = dynamic_cast<mTableLayout*>(parent)) {
      tableLayout->synchronize();
    }
  }
  return *this;
}

//

auto mTableLayoutRow::alignment() const -> Alignment {
  return state.alignment;
}

auto mTableLayoutRow::setAlignment(Alignment alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mTableLayoutRow::setSpacing(f32 spacing) -> type& {
  state.spacing = spacing;
  return synchronize();
}

auto mTableLayoutRow::spacing() const -> f32 {
  return state.spacing;
}

auto mTableLayoutRow::synchronize() -> type& {
  if(auto parent = this->parent()) {
    if(auto tableLayout = dynamic_cast<mTableLayout*>(parent)) {
      tableLayout->synchronize();
    }
  }
  return *this;
}

//

auto mTableLayoutCell::alignment() const -> Alignment {
  return state.alignment;
}

auto mTableLayoutCell::destruct() -> void {
  if(auto& sizable = state.sizable) sizable->destruct();
  mObject::destruct();
}

auto mTableLayoutCell::setAlignment(Alignment alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mTableLayoutCell::setEnabled(bool enabled) -> type& {
  mObject::setEnabled(enabled);
  state.sizable->setEnabled(state.sizable->enabled());
  return *this;
}

auto mTableLayoutCell::setFont(const Font& font) -> type& {
  mObject::setFont(font);
  state.sizable->setFont(state.sizable->font());
  return *this;
}

auto mTableLayoutCell::setParent(mObject* parent, s32 offset) -> type& {
  state.sizable->destruct();
  mObject::setParent(parent, offset);
  state.sizable->setParent(this, 0);
  return *this;
}

auto mTableLayoutCell::setSizable(sSizable sizable) -> type& {
  state.sizable = sizable;
  state.sizable->setParent(this, 0);
  return synchronize();
}

auto mTableLayoutCell::setSize(Size size) -> type& {
  state.size = size;
  return synchronize();
}

auto mTableLayoutCell::setVisible(bool visible) -> type& {
  mObject::setVisible(visible);
  state.sizable->setVisible(state.sizable->visible());
  return *this;
}

auto mTableLayoutCell::sizable() const -> Sizable {
  return state.sizable ? state.sizable : Sizable();
}

auto mTableLayoutCell::size() const -> Size {
  return state.size;
}

auto mTableLayoutCell::synchronize() -> type& {
  if(auto parent = this->parent()) {
    if(auto tableLayout = dynamic_cast<mTableLayout*>(parent)) {
      tableLayout->synchronize();
    }
  }
  return *this;
}

#endif
