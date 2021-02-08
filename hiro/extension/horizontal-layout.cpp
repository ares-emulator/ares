#if defined(Hiro_HorizontalLayout)

auto mHorizontalLayout::alignment() const -> maybe<f32> {
  return state.alignment;
}

auto mHorizontalLayout::append(sSizable sizable, Size size, f32 spacing) -> type& {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return *this;
  }

  HorizontalLayoutCell cell;
  cell->setSizable(sizable);
  cell->setSize(size);
  cell->setSpacing(spacing);
  cell->setParent(this, cellCount());
  state.cells.append(cell);
  return synchronize();
}

auto mHorizontalLayout::cell(u32 position) const -> HorizontalLayoutCell {
  return state.cells(position, {});
}

auto mHorizontalLayout::cell(sSizable sizable) const -> HorizontalLayoutCell {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return cell;
  }
  return {};
}

auto mHorizontalLayout::cells() const -> vector<HorizontalLayoutCell> {
  return state.cells;
}

auto mHorizontalLayout::cellCount() const -> u32 {
  return state.cells.size();
}

auto mHorizontalLayout::destruct() -> void {
  for(auto& cell : state.cells) cell->destruct();
  mSizable::destruct();
}

auto mHorizontalLayout::minimumSize() const -> Size {
  f32 width = 0;
  f32 spacing = 0;
  for(auto index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(cell.size().width() == Size::Minimum || cell.size().width() == Size::Maximum) {
      width += cell.sizable().minimumSize().width();
    } else {
      width += cell.size().width();
    }
    width += spacing;
    spacing = cell.spacing();
  }

  f32 height = 0;
  for(auto index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(cell.size().height() == Size::Minimum || cell.size().height() == Size::Maximum) {
      height = max(height, cell.sizable().minimumSize().height());
      continue;
    }
    height = max(height, cell.size().height());
  }

  return {
    padding().x() + width  + padding().width(),
    padding().y() + height + padding().height()
  };
}

auto mHorizontalLayout::padding() const -> Geometry {
  return state.padding;
}

auto mHorizontalLayout::remove(sSizable sizable) -> type& {
  for(auto& cell : state.cells) {
    if(cell->state.sizable == sizable) return remove(cell);
  }
  return *this;
}

auto mHorizontalLayout::remove(sHorizontalLayoutCell cell) -> type& {
  if(cell->parent() != this) return *this;
  auto offset = cell->offset();
  cell->setParent();
  state.cells.remove(offset);
  for(u32 n : range(offset, cellCount())) state.cells[n]->adjustOffset(-1);
  return synchronize();
}

auto mHorizontalLayout::reset() -> type& {
  while(state.cells) remove(state.cells.right());
  return synchronize();
}

auto mHorizontalLayout::resize() -> type& {
  setGeometry(geometry());
  return *this;
}

auto mHorizontalLayout::setAlignment(maybe<f32> alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mHorizontalLayout::setEnabled(bool enabled) -> type& {
  mSizable::setEnabled(enabled);
  for(auto& cell : state.cells) cell.sizable().setEnabled(cell.sizable().enabled());
  return *this;
}

auto mHorizontalLayout::setFont(const Font& font) -> type& {
  mSizable::setFont(font);
  for(auto& cell : state.cells) cell.sizable().setFont(cell.sizable().font());
  return *this;
}

auto mHorizontalLayout::setGeometry(Geometry requestedGeometry) -> type& {
//if(!visible(true)) return mSizable::setGeometry(requestedGeometry), *this;

  auto geometry = requestedGeometry;
  geometry.setX(geometry.x() + padding().x());
  geometry.setY(geometry.y() + padding().y());
  geometry.setWidth (geometry.width()  - padding().x() - padding().width());
  geometry.setHeight(geometry.height() - padding().y() - padding().height());

  vector<f32> widths;
  widths.resize(cellCount());
  u32 maximumWidths = 0;
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    f32 width = 0;
    if(cell.size().width() == Size::Maximum) {
      width = Size::Maximum;
      maximumWidths++;
    } else if(cell.size().width() == Size::Minimum) {
      width = cell.sizable().minimumSize().width();
    } else {
      width = cell.size().width();
    }
    widths[index] = width;
  }

  f32 fixedWidth = 0;
  f32 spacing = 0;
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(widths[index] != Size::Maximum) fixedWidth += widths[index];
    fixedWidth += spacing;
    spacing = cell.spacing();
  }

  f32 maximumWidth = (geometry.width() - fixedWidth) / maximumWidths;
  for(auto& width : widths) {
    if(width == Size::Maximum) width = maximumWidth;
  }

  f32 height = 0;
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    if(cell.size().height() == Size::Maximum) {
      height = geometry.height();
      break;
    } else if(cell.size().height() == Size::Minimum) {
      height = max(height, cell.sizable().minimumSize().height());
    } else {
      height = max(height, cell.size().height());
    }
  }

  f32 geometryX = geometry.x();
  f32 geometryY = geometry.y();
  for(u32 index : range(cellCount())) {
    auto cell = this->cell(index);
    if(cell.collapsible()) continue;
    f32 geometryWidth  = widths[index];
    f32 geometryHeight = height;
    auto alignment = cell.alignment();
    if(!alignment) alignment = this->alignment();
    if(!alignment) alignment = 0.5;
    f32 cellWidth  = geometryWidth;
    f32 cellHeight = cell.size().height();
    if(cellHeight == Size::Minimum) cellHeight = cell.sizable()->minimumSize().height();
    if(cellHeight == Size::Maximum) cellHeight = geometryHeight;
    f32 cellX = geometryX;
    f32 cellY = geometryY + alignment() * (geometryHeight - cellHeight);
    cell.sizable().setGeometry({cellX, cellY, cellWidth, cellHeight});
    geometryX += geometryWidth + cell.spacing();
  }

  mSizable::setGeometry(requestedGeometry);
  return *this;
}

auto mHorizontalLayout::setPadding(Geometry padding) -> type& {
  state.padding = padding;
  return synchronize();
}

auto mHorizontalLayout::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& cell : reverse(state.cells)) cell->destruct();
  mSizable::setParent(parent, offset);
  for(auto& cell : state.cells) cell->setParent(this, cell->offset());
  return *this;
}

auto mHorizontalLayout::setSpacing(f32 spacing) -> type& {
  state.spacing = spacing;
  return synchronize();
}

auto mHorizontalLayout::setVisible(bool visible) -> type& {
  mSizable::setVisible(visible);
  for(auto& cell : state.cells) cell.sizable().setVisible(cell.sizable().visible());
  return synchronize();
}

auto mHorizontalLayout::spacing() const -> f32 {
  return state.spacing;
}

auto mHorizontalLayout::synchronize() -> type& {
  setGeometry(geometry());
  return *this;
}

//

auto mHorizontalLayoutCell::alignment() const -> maybe<f32> {
  return state.alignment;
}

auto mHorizontalLayoutCell::collapsible() const -> bool {
  if(state.sizable) return state.sizable->collapsible() && !state.sizable->visible();
  return false;
}

auto mHorizontalLayoutCell::destruct() -> void {
  if(auto& sizable = state.sizable) sizable->destruct();
  mObject::destruct();
}

auto mHorizontalLayoutCell::setAlignment(maybe<f32> alignment) -> type& {
  state.alignment = alignment;
  return synchronize();
}

auto mHorizontalLayoutCell::setEnabled(bool enabled) -> type& {
  mObject::setEnabled(enabled);
  state.sizable->setEnabled(state.sizable->enabled());
  return *this;
}

auto mHorizontalLayoutCell::setFont(const Font& font) -> type& {
  mObject::setFont(font);
  state.sizable->setFont(state.sizable->font());
  return *this;
}

auto mHorizontalLayoutCell::setParent(mObject* parent, s32 offset) -> type& {
  state.sizable->destruct();
  mObject::setParent(parent, offset);
  state.sizable->setParent(this, 0);
  return *this;
}

auto mHorizontalLayoutCell::setSizable(sSizable sizable) -> type& {
  state.sizable = sizable;
  return synchronize();
}

auto mHorizontalLayoutCell::setSize(Size size) -> type& {
  state.size = size;
  return synchronize();
}

auto mHorizontalLayoutCell::setSpacing(f32 spacing) -> type& {
  state.spacing = spacing;
  return synchronize();
}

auto mHorizontalLayoutCell::setVisible(bool visible) -> type& {
  mObject::setVisible(visible);
  state.sizable->setVisible(state.sizable->visible());
  return *this;
}

auto mHorizontalLayoutCell::sizable() const -> Sizable {
  return state.sizable ? state.sizable : Sizable();
}

auto mHorizontalLayoutCell::size() const -> Size {
  return state.size;
}

auto mHorizontalLayoutCell::spacing() const -> f32 {
  return state.spacing;
}

auto mHorizontalLayoutCell::synchronize() -> type& {
  if(auto parent = this->parent()) {
    if(auto horizontalLayout = dynamic_cast<mHorizontalLayout*>(parent)) {
      horizontalLayout->synchronize();
    }
  }
  return *this;
}

#endif
