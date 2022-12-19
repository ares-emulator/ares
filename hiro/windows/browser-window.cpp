#if defined(Hiro_BrowserWindow)

namespace hiro {

static auto CALLBACK BrowserWindowCallbackProc(HWND hwnd, UINT msg, LPARAM lparam, LPARAM lpdata) -> s32 {
  if(msg == BFFM_INITIALIZED) {
    if(lpdata) {
      auto state = (BrowserWindow::State*)lpdata;
      utf16_t wpath(string{state->path}.transform("/", "\\"));
      if(state->title) SetWindowText(hwnd, utf16_t(state->title));
      SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(wchar_t*)wpath);
    }
  }
  return 0;
}

static auto BrowserWindow_fileDialog(bool save, BrowserWindow::State& state) -> string {
  string path = string{state.path}.replace("/", "\\");

  string filters;
  for(auto& filter : state.filters) {
    auto part = filter.split("|", 1L);
    if(part.size() != 2) continue;
    filters.append(part[0], "\t", part[1].transform(":", ";"), "\t");
  }

  utf16_t wfilters(filters);
  wchar_t wname[PATH_MAX + 1] = L"";
  utf16_t wpath(path);
  utf16_t wtitle(state.title);

  wchar_t* p = wfilters;
  while(*p != L'\0') {
    if(*p == L'\t') *p = L'\0';
    p++;
  }

  if(path) {
    //clear COMDLG32 MRU (most recently used) file list
    //this is required in order for lpstrInitialDir to be honored in Windows 7 and above
    registry::remove("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedPidlMRU\\");
    registry::remove("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\\");
  }

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = state.parent ? state.parent->self()->hwnd : 0;
  ofn.lpstrFilter = wfilters;
  ofn.lpstrInitialDir = wpath;
  ofn.lpstrFile = wname;
  ofn.lpstrTitle = wtitle;
  ofn.nMaxFile = PATH_MAX;
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = L"";

  bool result = (save == false ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn));
  if(result == false) return "";
  string name = (const char*)utf8_t(wname);
  name.transform("\\", "/");
  return name;
}

auto pBrowserWindow::directory(BrowserWindow::State& state) -> string {
  wchar_t wname[PATH_MAX + 1] = L"";

  BROWSEINFO bi;
  bi.hwndOwner = state.parent ? state.parent->self()->hwnd : 0;
  bi.pidlRoot = NULL;
  bi.pszDisplayName = wname;
  bi.lpszTitle = L"\nChoose a directory:";
  bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
  bi.lpfn = BrowserWindowCallbackProc;
  bi.lParam = (LPARAM)&state;
  bi.iImage = 0;
  bool result = false;
  LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
  if(pidl) {
    if(SHGetPathFromIDList(pidl, wname)) {
      result = true;
      IMalloc *imalloc = 0;
      if(SUCCEEDED(SHGetMalloc(&imalloc))) {
        imalloc->Free(pidl);
        imalloc->Release();
      }
    }
  }
  if(result == false) return "";
  string name = (const char*)utf8_t(wname);
  if(!name) return "";
  name.transform("\\", "/");
  if(name.endsWith("/") == false) name.append("/");
  return name;
}

auto pBrowserWindow::open(BrowserWindow::State& state) -> string {
  return BrowserWindow_fileDialog(0, state);
}

auto pBrowserWindow::save(BrowserWindow::State& state) -> string {
  return BrowserWindow_fileDialog(1, state);
}

}

#endif
