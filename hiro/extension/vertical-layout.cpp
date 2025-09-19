#if defined(Hiro_VerticalLayout)

auto mVerticalLayout::alignment() const -> maybe<f32> {
  return state.alignment;
}

auto mVerticalLayout::append(sSizable sizable, Size size, f32 spacing) -> type& {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return *this;
  }

  VerticalLayoutCell cell;
  cell->setSizable(sizable);
  cell->setSize(size);
  cell->setSpacing(spacing);
  cell->setParent(this, cellCount());
  state.cells.push_back(cell);
  return synchronize();
}

auto mVerticalLayout::cell(u32 position) const -> VerticalLayoutCell {
  if(position < state.cells.size()) return state.cells[position];
  return {};
}

auto mVerticalLayout::cell(sSizable sizable) const -> VerticalLayoutCell {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return cell;
  }
  return {};
}

auto mVerticalLayout::cells() const -> std::vector<VerticalLayoutCell> {
  return state.cells;
}

auto mVerticalLayout::cellCount() const -> u32 {
  return state.cells.size();
}

auto mVerticalLayout::destruct() -> void {
  for(auto& cell : state.cells) cell->destruct();
  mSizable::destruct();
}

auto mVerticalLayout::minimumSize() const -> Size {
  f32 width = 0;
  for(auto index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(cell.size().width() == Size::Minimum || cell.size().width() == Size::Maximum) {
      width = max(width, cell.sizable().minimumSize().width());
      continue;
    }
    width = max(width, cell.size().width());
  }

  f32 height = 0;
  f32 spacing = 0;
  for(auto index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(cell.size().height() == Size::Minimum || cell.size().height() == Size::Maximum) {
      height += cell.sizable().minimumSize().height();
    } else {
      height += cell.size().height();
    }
    height += spacing;
    spacing = cell.spacing();
  }

  return {
    padding().x() + width  + padding().width(),
    padding().y() + height + padding().height()
  };
}

auto mVerticalLayout::padding() const -> Geometry {
  return state.padding;
}

auto mVerticalLayout::remove(sSizable sizable) -> type& {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return remove(cell);
  }
  return *this;
}

auto mVerticalLayout::remove(sVerticalLayoutCell cell) -> type& {
  if(cell->parent() != this) return *this;
  auto offset = cell->offset();
  cell->setParent();
  state.cells.erase(state.cells.begin() + offset);
  for(u32 n : range(offset, cellCount())) state.cells[n]->adjustOffset(-1);
  return synchronize();
}

auto mVerticalLayout::reset() -> type& {
  while(!state.cells.empty()) remove(state.cells.back());
  return synchronize();
}

auto mVerticalLayout::resize() -> type& {
  setGeometry(geometry());
  return *this;
}

auto mVerticalLayout::setAlignment(maybe<f32> alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mVerticalLayout::setEnabled(bool enabled) -> type& {
  mSizable::setEnabled(enabled);
  for(auto& cell : state.cells) cell.sizable().setEnabled(cell.sizable().enabled());
  return *this;
}

auto mVerticalLayout::setFont(const Font& font) -> type& {
  mSizable::setFont(font);
  for(auto& cell : state.cells) cell.sizable().setFont(cell.sizable().font());
  return *this;
}

auto mVerticalLayout::setGeometry(Geometry requestedGeometry) -> type& {
//if(!visible(true)) return mSizable::setGeometry(requestedGeometry), *this;

  auto geometry = requestedGeometry;
  geometry.setX(geometry.x() + padding().x());
  geometry.setY(geometry.y() + padding().y());
  geometry.setWidth (geometry.width()  - padding().x() - padding().width());
  geometry.setHeight(geometry.height() - padding().y() - padding().height());

  f32 width = 0;
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(cell.size().width() == Size::Maximum) {
      width = geometry.width();
      break;
    } else if(cell.size().width() == Size::Minimum) {
      width = max(width, cell.sizable().minimumSize().width());
    } else {
      width = max(width, cell.size().width());
    }
  }

  std::vector<f32> heights;
  heights.resize(cellCount());
  u32 maximumHeights = 0;
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    f32 height = 0;
    if(cell.size().height() == Size::Maximum) {
      height = Size::Maximum;
      maximumHeights++;
    } else if(cell.size().height() == Size::Minimum) {
      height = cell.sizable().minimumSize().height();
    } else {
      height = cell.size().height();
    }
    heights[index] = height;
  }

  f32 fixedHeight = 0;
  f32 spacing = 0;
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(heights[index] != Size::Maximum) fixedHeight += heights[index];
    fixedHeight += spacing;
    spacing = cell.spacing();
  }

  f32 maximumHeight = (geometry.height() - fixedHeight) / maximumHeights;
  for(auto& height : heights) {
    if(height == Size::Maximum) height = maximumHeight;
  }

  f32 geometryX = geometry.x();
  f32 geometryY = geometry.y();
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    f32 geometryWidth  = width;
    f32 geometryHeight = heights[index];
    auto alignment = cell.alignment();
    if(!alignment) alignment = this->alignment();
    if(!alignment) alignment = 0.0;
    f32 cellWidth  = cell.size().width();
    f32 cellHeight = geometryHeight;
    if(cellWidth == Size::Minimum) cellWidth = cell.sizable()->minimumSize().width();
    if(cellWidth == Size::Maximum) cellWidth = geometryWidth;
    f32 cellX = geometryX + alignment() * (geometryWidth - cellWidth);
    f32 cellY = geometryY;
    cell.sizable().setGeometry({cellX, cellY, cellWidth, cellHeight});
    geometryY += geometryHeight + cell.spacing();
  }

  mSizable::setGeometry(requestedGeometry);
  return *this;
}

auto mVerticalLayout::setPadding(Geometry padding) -> type& {
  state.padding = padding;
  return synchronize();
}

auto mVerticalLayout::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& cell : state.cells | std::views::reverse) cell->destruct();
  mSizable::setParent(parent, offset);
  for(auto& cell : state.cells) cell->setParent(this, cell->offset());
  return *this;
}

auto mVerticalLayout::setSpacing(f32 spacing) -> type& {
  state.spacing = spacing;
  return synchronize();
}

auto mVerticalLayout::setVisible(bool visible) -> type& {
  mSizable::setVisible(visible);
  for(auto& cell : state.cells) cell.sizable().setVisible(cell.sizable().visible());
  return synchronize();
}

auto mVerticalLayout::spacing() const -> f32 {
  return state.spacing;
}

auto mVerticalLayout::synchronize() -> type& {
  setGeometry(geometry());
  return *this;
}

//

auto mVerticalLayoutCell::alignment() const -> maybe<f32> {
  return state.alignment;
}

auto mVerticalLayoutCell::collapsible() const -> bool {
  if(state.sizable) return state.sizable->collapsible() && !state.sizable->visible();
  return false;
}

auto mVerticalLayoutCell::destruct() -> void {
  if(auto& sizable = state.sizable) sizable->destruct();
  mObject::destruct();
}

auto mVerticalLayoutCell::setAlignment(maybe<f32> alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mVerticalLayoutCell::setEnabled(bool enabled) -> type& {
  mObject::setEnabled(enabled);
  state.sizable->setEnabled(state.sizable->enabled());
  return *this;
}

auto mVerticalLayoutCell::setFont(const Font& font) -> type& {
  mObject::setFont(font);
  state.sizable->setFont(state.sizable->font());
  return *this;
}

auto mVerticalLayoutCell::setParent(mObject* parent, s32 offset) -> type& {
  state.sizable->destruct();
  mObject::setParent(parent, offset);
  state.sizable->setParent(this, 0);
  return *this;
}

auto mVerticalLayoutCell::setSizable(sSizable sizable) -> type& {
  state.sizable = sizable;
  return synchronize();
}

auto mVerticalLayoutCell::setSize(Size size) -> type& {
  state.size = size;
  return synchronize();
}

auto mVerticalLayoutCell::setSpacing(f32 spacing) -> type& {
  state.spacing = spacing;
  return synchronize();
}

auto mVerticalLayoutCell::setVisible(bool visible) -> type& {
  mObject::setVisible(visible);
  state.sizable->setVisible(state.sizable->visible());
  return *this;
}

auto mVerticalLayoutCell::sizable() const -> Sizable {
  return state.sizable ? state.sizable : Sizable();
}

auto mVerticalLayoutCell::size() const -> Size {
  return state.size;
}

auto mVerticalLayoutCell::spacing() const -> f32 {
  return state.spacing;
}

auto mVerticalLayoutCell::synchronize() -> type& {
  if(auto parent = this->parent()) {
    if(auto verticalLayout = dynamic_cast<mVerticalLayout*>(parent)) {
      verticalLayout->synchronize();
    }
  }
  return *this;
}

#endif
