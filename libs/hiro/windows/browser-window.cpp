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
    auto part = nall::split(filter, "|", 1L);
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
  using namespace Microsoft::WRL; // For ComPtr

  ComPtr<IFileDialog> pfd;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

  if (FAILED(hr)) {
    return "";
  }

  DWORD dwOptions;
  hr = pfd->GetOptions(&dwOptions);
  if (FAILED(hr)) {
    return "";
  }

  hr = pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
  if (FAILED(hr)) {
    return "";
  }

  hr = pfd->Show(NULL);
  if (FAILED(hr)) {
    return "";
  }

  ComPtr<IShellItem> psi;
  hr = pfd->GetResult(&psi);
  if (FAILED(hr)) {
    return "";
  }

  PWSTR pszPath;
  hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
  if (FAILED(hr)) {
    return "";
  }

  string name = (const char*)utf8_t(pszPath);
  CoTaskMemFree(pszPath);

  if (!name) return "";
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
