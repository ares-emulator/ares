#if defined(Hiro_HexEdit)

namespace hiro {

auto pHexEdit::construct() -> void {
  hwnd = CreateWindowEx(
    WS_EX_CLIENTEDGE, L"EDIT", L"",
    WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY | ES_MULTILINE | ES_WANTRETURN,
    0, 0, 0, 0, _parentHandle(), nullptr, GetModuleHandle(0), 0
  );
  scrollBar = CreateWindowEx(
    0, L"SCROLLBAR", L"",
    WS_VISIBLE | WS_CHILD | SBS_VERT,
    0, 0, 0, 0, hwnd, nullptr, GetModuleHandle(0), 0
  );
  SetWindowLongPtr(scrollBar, GWLP_USERDATA, (LONG_PTR)&reference);
  pWidget::construct();

  setAddress(state().address);
  setBackgroundColor(state().backgroundColor);
  setLength(state().length);
  update();
  PostMessage(hwnd, EM_SETSEL, 10, 10);
}

auto pHexEdit::destruct() -> void {
  DestroyWindow(hwnd);
}

auto pHexEdit::setAddress(u32 address) -> void {
  SetScrollPos(scrollBar, SB_CTL, address / state().columns, true);
  update();
}

auto pHexEdit::setBackgroundColor(Color color) -> void {
  if(backgroundBrush) DeleteObject(backgroundBrush);
  backgroundBrush = CreateSolidBrush(color ? CreateRGB(color) : GetSysColor(COLOR_WINDOW));
}

auto pHexEdit::setColumns(u32 columns) -> void {
  update();
}

auto pHexEdit::setForegroundColor(Color color) -> void {
}

auto pHexEdit::setLength(u32 length) -> void {
  SetScrollRange(scrollBar, SB_CTL, 0, rowsScrollable(), true);
  EnableWindow(scrollBar, rowsScrollable() > 0);
  update();
}

auto pHexEdit::setRows(u32 rows) -> void {
  update();
}

auto pHexEdit::update() -> void {
  if(!state().onRead) {
    SetWindowText(hwnd, L"");
    return;
  }

  u32 cursorPosition = Edit_GetSel(hwnd);

  string output;
  u32 address = state().address;
  for(auto row : range(state().rows)) {
    output.append(hex(address, 8L));
    output.append("  ");

    string hexdata;
    string ansidata = " ";
    for(auto column : range(state().columns)) {
      if(address < state().length) {
        u8 data = self().doRead(address++);
        hexdata.append(hex(data, 2L));
        hexdata.append(" ");
        ansidata.append(data >= 0x20 && data <= 0x7e ? (char)data : '.');
      } else {
        hexdata.append("   ");
        ansidata.append(" ");
      }
    }

    output.append(hexdata);
    output.append(ansidata);
    if(address >= state().length) break;
    if(row != state().rows - 1) output.append("\r\n");
  }

  SetWindowText(hwnd, utf16_t(output));
  Edit_SetSel(hwnd, LOWORD(cursorPosition), HIWORD(cursorPosition));
}

auto pHexEdit::keyPress(u32 scancode) -> bool {
  if(!state().onRead) return false;

  s32 position = LOWORD(Edit_GetSel(hwnd));
  s32 lineWidth = 10 + (state().columns * 3) + 1 + state().columns + 2;
  s32 cursorY = position / lineWidth;
  s32 cursorX = position % lineWidth;

  if(scancode == VK_HOME) {
    s32 offset = cursorY * lineWidth + 10;
    Edit_SetSel(hwnd, offset, offset);
    return true;
  }

  if(scancode == VK_END) {
    s32 offset = cursorY * lineWidth + 57;
    Edit_SetSel(hwnd, offset, offset);
    return true;
  }

  if(scancode == VK_UP) {
    if(cursorY > 0) return false;
    scrollTo(scrollPosition() - 1);
    return true;
  }

  if(scancode == VK_DOWN) {
    if(cursorY >= rows() - 1) return true;
    if(cursorY < state().rows - 1) return false;
    scrollTo(scrollPosition() + 1);
    return true;
  }

  if(scancode == VK_PRIOR) {
    scrollTo(scrollPosition() - state().rows);
    return true;
  }

  if(scancode == VK_NEXT) {
    scrollTo(scrollPosition() + state().rows);
    return true;
  }

  //convert scancode to hex nibble
       if(scancode >= '0' && scancode <= '9') scancode = scancode - '0';
  else if(scancode >= 'A' && scancode <= 'F') scancode = scancode - 'A' + 10;
  else if(scancode >= 'a' && scancode <= 'f') scancode = scancode - 'a' + 10;
  else return false;

  if(cursorX >= 10) {
    //not on an address
    cursorX -= 10;
    if((cursorX % 3) != 2) {
      //not on a space
      bool cursorNibble = (cursorX % 3) == 1;  //0 = high, 1 = low
      cursorX /= 3;
      if(cursorX < state().columns) {
        //not in ANSI region
        u32 address = state().address + (cursorY * state().columns + cursorX);

        if(address >= state().length) return false;  //do not edit past end of data
        u8 data = self().doRead(address);

        //write modified value
        if(cursorNibble == 1) {
          data = (data & 0xf0) | (scancode << 0);
        } else {
          data = (data & 0x0f) | (scancode << 4);
        }
        self().doWrite(address, data);

        //auto-advance cursor to next nibble or byte
        position++;
        if(cursorNibble && cursorX != state().columns - 1) position++;
        Edit_SetSel(hwnd, position, position);

        //refresh output to reflect modified data
        update();
      }
    }
  }

  return true;
}

auto pHexEdit::rows() -> s32 {
  return (max(1u, state().length) + state().columns - 1) / state().columns;
}

auto pHexEdit::rowsScrollable() -> s32 {
  return max(0u, rows() - state().rows);
}

auto pHexEdit::scrollPosition() -> s32 {
  return state().address / state().columns;
}

auto pHexEdit::scrollTo(s32 position) -> void {
  if(position > rowsScrollable()) position = rowsScrollable();
  if(position < 0) position = 0;
  if(position == scrollPosition()) return;
  self().setAddress(position * state().columns);
}

auto pHexEdit::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> maybe<LRESULT> {
  if(msg == WM_KEYDOWN) {
    if(keyPress(wparam)) return 0;
  }

  if(msg == WM_MOUSEWHEEL) {
    s32 offset = -((s16)HIWORD(wparam) / WHEEL_DELTA);
    scrollTo(scrollPosition() + offset);
    return true;
  }

  if(msg == WM_SIZE) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    SetWindowPos(scrollBar, HWND_TOP, rc.right - 18, 0, 18, rc.bottom, SWP_SHOWWINDOW);
  }

  if(msg == WM_VSCROLL) {
    SCROLLINFO info{sizeof(SCROLLINFO)};
    info.fMask = SIF_ALL;
    GetScrollInfo((HWND)lparam, SB_CTL, &info);
    switch(LOWORD(wparam)) {
    case SB_LEFT: info.nPos = info.nMin; break;
    case SB_RIGHT: info.nPos = info.nMax; break;
    case SB_LINELEFT: info.nPos--; break;
    case SB_LINERIGHT: info.nPos++; break;
    case SB_PAGELEFT: info.nPos -= info.nMax >> 3; break;
    case SB_PAGERIGHT: info.nPos += info.nMax >> 3; break;
    case SB_THUMBTRACK: info.nPos = info.nTrackPos; break;
    }

    info.fMask = SIF_POS;
    SetScrollInfo((HWND)lparam, SB_CTL, &info, TRUE);
    GetScrollInfo((HWND)lparam, SB_CTL, &info);  //get clamped position

    scrollTo(info.nPos);
    return true;
  }

  return pWidget::windowProc(hwnd, msg, wparam, lparam);
}

}

#endif
