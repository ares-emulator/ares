#if defined(Hiro_TableView)

namespace hiro {

auto pTableView::construct() -> void {
  hwnd = CreateWindowEx(
    WS_EX_CLIENTEDGE | LVS_EX_DOUBLEBUFFER, WC_LISTVIEW, L"",
    WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
    0, 0, 0, 0, _parentHandle(), nullptr, GetModuleHandle(0), 0
  );
  ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
  pWidget::construct();
  setBackgroundColor(state().backgroundColor);
  setBatchable(state().batchable);
  setBordered(state().bordered);
  setHeadered(state().headered);
  setSortable(state().sortable);
  _setIcons();
  resizeColumns();
}

auto pTableView::destruct() -> void {
  if(imageList) { ImageList_Destroy(imageList); imageList = nullptr; }
  DestroyWindow(hwnd);
}

auto pTableView::append(sTableViewColumn column) -> void {
  resizeColumns();
}

auto pTableView::append(sTableViewItem item) -> void {
}

auto pTableView::remove(sTableViewColumn column) -> void {
}

auto pTableView::remove(sTableViewItem item) -> void {
}

auto pTableView::resizeColumns() -> void {
  auto lock = acquire();

  vector<s32> widths;
  s32 minimumWidth = 0;
  s32 expandable = 0;
  for(auto column : range(self().columnCount())) {
    s32 width = _width(column);
    widths.append(width);
    minimumWidth += width;
    if(state().columns[column]->expandable()) expandable++;
  }

  s32 maximumWidth = self().geometry().width() - 4;
  SCROLLBARINFO sbInfo{sizeof(SCROLLBARINFO)};
  if(GetScrollBarInfo(hwnd, OBJID_VSCROLL, &sbInfo)) {
    if(!(sbInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE)) {
      maximumWidth -= sbInfo.rcScrollBar.right - sbInfo.rcScrollBar.left;
    }
  }

  s32 expandWidth = 0;
  if(expandable && maximumWidth > minimumWidth) {
    expandWidth = (maximumWidth - minimumWidth) / expandable;
  }

  for(auto column : range(self().columnCount())) {
    if(auto self = state().columns[column]->self()) {
      s32 width = widths[column];
      if(self->state().expandable) width += expandWidth;
      self->_width = width;
      self->_setState();
    }
  }
}

auto pTableView::setAlignment(Alignment alignment) -> void {
}

auto pTableView::setBackgroundColor(Color color) -> void {
  if(!color) color = {255, 255, 255};
  ListView_SetBkColor(hwnd, RGB(color.red(), color.green(), color.blue()));
}

auto pTableView::setBatchable(bool batchable) -> void {
  auto style = GetWindowLong(hwnd, GWL_STYLE);
  !batchable ? style |= LVS_SINGLESEL : style &=~ LVS_SINGLESEL;
  SetWindowLong(hwnd, GWL_STYLE, style);
}

auto pTableView::setBordered(bool bordered) -> void {
  //rendered via onCustomDraw
}

auto pTableView::setForegroundColor(Color color) -> void {
}

auto pTableView::setGeometry(Geometry geometry) -> void {
  pWidget::setGeometry(geometry);
  for(auto& column : state().columns) {
    if(column->state.expandable) return resizeColumns();
  }
}

auto pTableView::onActivate(LPARAM lparam) -> void {
  auto nmlistview = (LPNMLISTVIEW)lparam;
  if(ListView_GetSelectedCount(hwnd) == 0) return;
  if(!locked()) {
    activateCell = TableViewCell();
    LVHITTESTINFO hitTest{};
    GetCursorPos(&hitTest.pt);
    ScreenToClient(nmlistview->hdr.hwndFrom, &hitTest.pt);
    ListView_SubItemHitTest(nmlistview->hdr.hwndFrom, &hitTest);
    if(hitTest.flags & LVHT_ONITEM) {
      s32 row = hitTest.iItem;
      if(row >= 0 && row < state().items.size()) {
        s32 column = hitTest.iSubItem;
        if(column >= 0 && column < state().columns.size()) {
          auto item = state().items[row];
          activateCell = item->cell(column);
        }
      }
    }
    //LVN_ITEMACTIVATE is not re-entrant until DispatchMessage() completes
    //thus, we don't call self().doActivate() here
    PostMessageOnce(_parentHandle(), AppMessage::TableView_onActivate, 0, (LPARAM)&reference);
  }
}

auto pTableView::onChange(LPARAM lparam) -> void {
  auto nmlistview = (LPNMLISTVIEW)lparam;
  if(!(nmlistview->uChanged & LVIF_STATE)) return;

  bool modified = false;
  for(auto& item : state().items) {
    bool selected = ListView_GetItemState(hwnd, item->offset(), LVIS_SELECTED) & LVIS_SELECTED;
    if(item->state.selected != selected) {
      modified = true;
      item->state.selected = selected;
    }
  }
  if(modified && !locked()) {
    //state event change messages are sent for every item
    //so when doing a batch select/deselect; this can generate several messages
    //we use a delayed AppMessage so that only one callback event is fired off
    PostMessageOnce(_parentHandle(), AppMessage::TableView_onChange, 0, (LPARAM)&reference);
  }
}

auto pTableView::onContext(LPARAM lparam) -> void {
  auto nmitemactivate = (LPNMITEMACTIVATE)lparam;
  if(ListView_GetSelectedCount(hwnd) > 0) {
    LVHITTESTINFO hitTest{};
    GetCursorPos(&hitTest.pt);
    ScreenToClient(nmitemactivate->hdr.hwndFrom, &hitTest.pt);
    ListView_SubItemHitTest(nmitemactivate->hdr.hwndFrom, &hitTest);
    if(hitTest.flags & LVHT_ONITEM) {
      s32 row = hitTest.iItem;
      if(row >= 0 && row < state().items.size()) {
        s32 column = hitTest.iSubItem;
        if(column >= 0 && column < state().columns.size()) {
          auto item = state().items[row];
          return self().doContext(item->cell(column));
        }
      }
    }
  }
  return self().doContext(TableViewCell());
}

auto pTableView::onCustomDraw(LPARAM lparam) -> LRESULT {
  auto lvcd = (LPNMLVCUSTOMDRAW)lparam;

  switch(lvcd->nmcd.dwDrawStage) {
  default: return CDRF_DODEFAULT;
  case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
  case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW | CDRF_NOTIFYPOSTPAINT;
  case CDDS_ITEMPREPAINT | CDDS_SUBITEM: return CDRF_SKIPDEFAULT;
  case CDDS_ITEMPOSTPAINT: {
    HDC hdc = lvcd->nmcd.hdc;
    HDC hdcSource = CreateCompatibleDC(hdc);
    u32 row = lvcd->nmcd.dwItemSpec;
    for(auto column : range(self().columnCount())) {
      RECT rc, rcLabel;
      ListView_GetSubItemRect(hwnd, row, column, LVIR_BOUNDS, &rc);
      ListView_GetSubItemRect(hwnd, row, column, LVIR_LABEL, &rcLabel);
      rc.right = rcLabel.right;  //bounds of column 0 returns width of entire item
      s32 iconSize = rc.bottom - rc.top - 1;
      bool selected = state().items[row]->state.selected;

      if(auto cell = self().item(row)->cell(column)) {
        auto backgroundColor = cell->backgroundColor(true);
        HBRUSH brush = CreateSolidBrush(
          selected ? GetSysColor(COLOR_HIGHLIGHT)
        : backgroundColor ? CreateRGB(backgroundColor)
        : GetSysColor(COLOR_WINDOW)
        );
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);

        if(cell->state.checkable) {
          if(auto htheme = OpenThemeData(hwnd, L"BUTTON")) {
            u32 state = cell->state.checked ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL;
            SIZE size;
            GetThemePartSize(htheme, hdc, BP_CHECKBOX, state, nullptr, TS_TRUE, &size);
            s32 center = max(0, (rc.bottom - rc.top - size.cy) / 2);
            RECT rd{rc.left + center, rc.top + center, rc.left + center + size.cx, rc.top + center + size.cy};
            DrawThemeBackground(htheme, hdc, BP_CHECKBOX, state, &rd, nullptr);
            CloseThemeData(htheme);
          } else {
            //Windows Classic
            rc.left += 2;
            RECT rd{rc.left, rc.top, rc.left + iconSize, rc.top + iconSize};
            DrawFrameControl(hdc, &rd, DFC_BUTTON, DFCS_BUTTONCHECK | (cell->state.checked ? DFCS_CHECKED : 0));
          }
          rc.left += iconSize + 2;
        } else {
          rc.left += 2;
        }

        if(auto& icon = cell->state.icon) {
          auto bitmap = CreateBitmap(icon);
          SelectBitmap(hdcSource, bitmap);
          BLENDFUNCTION blend{AC_SRC_OVER, 0, (BYTE)(selected ? 128 : 255), AC_SRC_ALPHA};
          AlphaBlend(hdc, rc.left, rc.top, iconSize, iconSize, hdcSource, 0, 0, icon.width(), icon.height(), blend);
          DeleteObject(bitmap);
          rc.left += iconSize + 2;
        }

        if(auto text = cell->state.text) {
          auto alignment = cell->alignment(true);
          if(!alignment) alignment = {0.0, 0.5};
          utf16_t wText(text);
          SetBkMode(hdc, TRANSPARENT);
          auto foregroundColor = cell->foregroundColor(true);
          SetTextColor(hdc,
            selected ? GetSysColor(COLOR_HIGHLIGHTTEXT)
          : foregroundColor ? CreateRGB(foregroundColor)
          : GetSysColor(COLOR_WINDOWTEXT)
          );
          auto style = DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;
          style |= alignment.horizontal() < 0.333 ? DT_LEFT : alignment.horizontal() > 0.666 ? DT_RIGHT  : DT_CENTER;
          style |= alignment.vertical()   < 0.333 ? DT_TOP  : alignment.vertical()   > 0.666 ? DT_BOTTOM : DT_VCENTER;
          rc.right -= 2;
          auto font = pFont::create(cell->font(true));
          SelectObject(hdc, font);
          DrawText(hdc, wText, -1, &rc, style);
          DeleteObject(font);
        }
      } else {
        auto backgroundColor = state().backgroundColor;
        HBRUSH brush = CreateSolidBrush(
          selected ? GetSysColor(COLOR_HIGHLIGHT)
        : backgroundColor ? CreateRGB(backgroundColor)
        : GetSysColor(COLOR_WINDOW)
        );
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
      }

      if(state().bordered) {
        ListView_GetSubItemRect(hwnd, row, column, LVIR_BOUNDS, &rc);
        rc.top = rc.bottom - 1;
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
        ListView_GetSubItemRect(hwnd, row, column, LVIR_LABEL, &rc);
        rc.left = rc.right - 1;
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
      }
    }
    DeleteDC(hdcSource);
    return CDRF_SKIPDEFAULT;
  }
  }

  return CDRF_SKIPDEFAULT;
}

auto pTableView::onSort(LPARAM lparam) -> void {
  auto nmlistview = (LPNMLISTVIEW)lparam;
  if(auto column = self().column(nmlistview->iSubItem)) {
    if(state().sortable) self().doSort(column);
  }
}

auto pTableView::onToggle(LPARAM lparam) -> void {
  auto itemActivate = (LPNMITEMACTIVATE)lparam;
  LVHITTESTINFO hitTestInfo{0};
  hitTestInfo.pt = itemActivate->ptAction;
  ListView_SubItemHitTest(hwnd, &hitTestInfo);

  if(auto cell = self().item(hitTestInfo.iItem).cell(hitTestInfo.iSubItem)) {
    if(cell->state.checkable) {
      cell->state.checked = !cell->state.checked;
      if(!locked()) self().doToggle(cell);
      //todo: try to find a way to only repaint this cell instead of the entire control to reduce flickering
      PostMessageOnce(_parentHandle(), AppMessage::TableView_doPaint, 0, (LPARAM)&reference);
    }
  }
}

auto pTableView::setHeadered(bool headered) -> void {
  auto style = GetWindowLong(hwnd, GWL_STYLE);
  headered ? style &=~ LVS_NOCOLUMNHEADER : style |= LVS_NOCOLUMNHEADER;
  SetWindowLong(hwnd, GWL_STYLE, style);
}

auto pTableView::setSortable(bool sortable) -> void {
  //note: this won't change the visual style: WC_LISTVIEW caches this in CreateWindow
  auto style = GetWindowLong(hwnd, GWL_STYLE);
  sortable ? style &=~ LVS_NOSORTHEADER : style |= LVS_NOSORTHEADER;
  SetWindowLong(hwnd, GWL_STYLE, style);
}

//

auto pTableView::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> maybe<LRESULT> {
  if(msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
    if(!self().enabled(true)) {
      //WC_LISTVIEW responds to key messages even when its HWND is disabled
      //the control should be inactive when disabled; so we intercept the messages here
      return false;
    }

    if(msg == WM_KEYDOWN && wparam == VK_RETURN) {
      if(self().selected()) {
        //returning true generates LVN_ITEMACTIVATE message
        return true;
      }
    }
  }

  //when hovering over a WC_LISTVIEW item, it will become selected after a very short pause (~200ms usually)
  //this is extremely annoying; so intercept the hover event and block it to suppress the LVN_ITEMCHANGING message
  if(msg == WM_MOUSEHOVER) {
    return false;
  }

  return pWidget::windowProc(hwnd, msg, wparam, lparam);
}

//

auto pTableView::_backgroundColor(u32 _row, u32 _column) -> Color {
  if(auto item = self().item(_row)) {
    if(auto cell = item->cell(_column)) {
      if(auto color = cell->backgroundColor()) return color;
    }
    if(auto color = item->backgroundColor()) return color;
  }
//if(auto column = self().column(_column)) {
//  if(auto color = column->backgroundColor()) return color;
//}
  if(auto color = self().backgroundColor()) return color;
//if(state().columns.size() >= 2 && _row % 2) return {240, 240, 240};
  return {255, 255, 255};
}

auto pTableView::_cellWidth(u32 _row, u32 _column) -> u32 {
  u32 width = 6;
  if(auto item = self().item(_row)) {
    if(auto cell = item->cell(_column)) {
      if(cell->state.checkable) {
        width += 16 + 2;
      }
      if(auto& icon = cell->state.icon) {
        width += 16 + 2;
      }
      if(auto& text = cell->state.text) {
        width += pFont::size(_font(_row, _column), text).width();
      }
    }
  }
  return width;
}

auto pTableView::_columnWidth(u32 _column) -> u32 {
  u32 width = 12;
  if(auto column = self().column(_column)) {
    if(auto& icon = column->state.icon) {
      width += 16 + 12;  //yes; icon spacing in column headers is excessive
    }
    if(auto& text = column->state.text) {
      width += pFont::size(self().font(true), text).width();
    }
    if(column->state.sorting != Sort::None) {
      width += 12;
    }
  }
  return width;
}

auto pTableView::_font(u32 _row, u32 _column) -> Font {
  if(auto item = self().item(_row)) {
    if(auto cell = item->cell(_column)) {
      if(auto font = cell->font()) return font;
    }
    if(auto font = item->font()) return font;
  }
//if(auto column = self().column(_column)) {
//  if(auto font = column->font()) return font;
//}
  if(auto font = self().font(true)) return font;
  return {};
}

auto pTableView::_foregroundColor(u32 _row, u32 _column) -> Color {
  if(auto item = self().item(_row)) {
    if(auto cell = item->cell(_column)) {
      if(auto color = cell->foregroundColor()) return color;
    }
    if(auto color = item->foregroundColor()) return color;
  }
//if(auto column = self().column(_column)) {
//  if(auto color = column->foregroundColor()) return color;
//}
  if(auto color = self().foregroundColor()) return color;
  return {0, 0, 0};
}

auto pTableView::_setIcons() -> void {
  ListView_SetImageList(hwnd, nullptr, LVSIL_SMALL);
  if(imageList) ImageList_Destroy(imageList);
  imageList = ImageList_Create(16, 16, ILC_COLOR32, 1, 0);
  ListView_SetImageList(hwnd, imageList, LVSIL_SMALL);

  for(auto column : range(self().columnCount())) {
    image icon;
    if(auto& sourceIcon = state().columns[column]->state.icon) {
      icon.allocate(sourceIcon.width(), sourceIcon.height());
      memory::copy(icon.data(), sourceIcon.data(), icon.size());
      icon.scale(16, 16);
    } else {
      icon.allocate(16, 16);
      icon.fill(0x00ffffff);
    }
    auto bitmap = CreateBitmap(icon);
    ImageList_Add(imageList, bitmap, nullptr);
    DeleteObject(bitmap);
  }

  //empty icon used for ListViewItems (drawn manually via onCustomDraw)
  image icon;
  icon.allocate(16, 16);
  icon.fill(0x00ffffff);
  auto bitmap = CreateBitmap(icon);
  ImageList_Add(imageList, bitmap, nullptr);
  DeleteObject(bitmap);
}

auto pTableView::_width(u32 column) -> u32 {
  if(auto width = self().column(column).width()) return width;
  u32 width = 1;
  if(state().headered) width = max(width, _columnWidth(column));
  for(auto row : range(state().items.size())) {
    width = max(width, _cellWidth(row, column));
  }
  return width;
}

}

#endif
