#if defined(Hiro_TreeView)

namespace hiro {

auto pTreeView::construct() -> void {
  hwnd = CreateWindowEx(
    WS_EX_CLIENTEDGE, WC_TREEVIEW, L"",
    WS_CHILD | WS_TABSTOP | TVS_HASLINES | TVS_SHOWSELALWAYS,
    0, 0, 0, 0, _parentHandle(), nullptr, GetModuleHandle(0), 0
  );
  TreeView_SetExtendedStyle(hwnd, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
  pWidget::construct();
  setActivation(state().activation);
  setBackgroundColor(state().backgroundColor);
  setForegroundColor(state().foregroundColor);
}

auto pTreeView::destruct() -> void {
  DestroyWindow(hwnd);
}

//

auto pTreeView::append(sTreeViewItem item) -> void {
}

auto pTreeView::remove(sTreeViewItem item) -> void {
}

auto pTreeView::setActivation(Mouse::Click activation) -> void {
}

auto pTreeView::setBackgroundColor(Color color) -> void {
  if(color) {
    TreeView_SetBkColor(hwnd, CreateRGB(color));
  }
}

auto pTreeView::setFocused() -> void {
}

auto pTreeView::setForegroundColor(Color color) -> void {
  if(color) {
    TreeView_SetTextColor(hwnd, CreateRGB(color));
  }
}

auto pTreeView::setGeometry(Geometry geometry) -> void {
}

//

auto pTreeView::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> maybe<LRESULT> {
  if(msg == WM_ERASEBKGND) {
    return 0;
  }

  if(msg == WM_GETDLGCODE) {
    if(wparam == VK_RETURN) return DLGC_WANTALLKEYS;
  }

  return pWidget::windowProc(hwnd, msg, wparam, lparam);
}

}

#endif
