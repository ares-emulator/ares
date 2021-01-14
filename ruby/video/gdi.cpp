static LRESULT CALLBACK VideoGDI_WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if(msg == WM_SYSKEYDOWN && wparam == VK_F4) return false;
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

struct VideoGDI : VideoDriver {
  VideoGDI& self = *this;
  VideoGDI(Video& super) : VideoDriver(super) { construct(); }
  ~VideoGDI() { destruct(); }

  auto create() -> bool override {
    VideoDriver::shader = "None";
    return initialize();
  }

  auto driver() -> string override { return "GDI"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override { return initialize(); }
  auto setMonitor(string monitor) -> bool override { return initialize(); }
  auto setContext(uintptr context) -> bool override { return initialize(); }

  auto focused() -> bool override {
    if(self.fullScreen && self.exclusive) return true;
    auto focused = GetFocus();
    return _context == focused || IsChild(_context, focused);
  }

  auto size(u32& width, u32& height) -> void override {
    RECT rectangle;
    GetClientRect(_context, &rectangle);
    width = rectangle.right - rectangle.left;
    height = rectangle.bottom - rectangle.top;
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(!_buffer || _width != width || _height != height) {
      if(_buffer) delete[] _buffer;
      if(_bitmap) DeleteObject(_bitmap);
      if(_dc) DeleteObject(_dc);

      _buffer = new u32[width * height]();
      _width = width;
      _height = height;

      HDC hdc = GetDC(_context);
      _dc = CreateCompatibleDC(hdc);
      _bitmap = CreateCompatibleBitmap(hdc, width, height);
      SelectObject(_dc, _bitmap);
      ReleaseDC(_context, hdc);

      memory::fill(&_info, sizeof(BITMAPINFO));
      _info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      _info.bmiHeader.biWidth = width;
      _info.bmiHeader.biHeight = -height;
      _info.bmiHeader.biPlanes = 1;
      _info.bmiHeader.biBitCount = 32;
      _info.bmiHeader.biCompression = BI_RGB;
      _info.bmiHeader.biSizeImage = width * height * sizeof(u32);
    }

    pitch = _width * sizeof(u32);
    return data = _buffer;
  }

  auto release() -> void override {
  }

  auto output(u32 width, u32 height) -> void override {
    u32 windowWidth, windowHeight;
    size(windowWidth, windowHeight);

    SetDIBits(_dc, _bitmap, 0, _height, (void*)_buffer, &_info, DIB_RGB_COLORS);
    HDC hdc = GetDC(_context);
    StretchBlt(hdc,
      ((s32)windowWidth - (s32)width) / 2, ((s32)windowHeight - (s32)height) / 2, width, height, _dc,
      0, 0, _width, _height, SRCCOPY
    );
    ReleaseDC(_context, hdc);
  }

private:
  auto construct() -> void {
    WNDCLASS windowClass{};
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hInstance = GetModuleHandle(0);
    windowClass.lpfnWndProc = VideoGDI_WindowProcedure;
    windowClass.lpszClassName = L"VideoGDI_Window";
    windowClass.lpszMenuName = 0;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&windowClass);
  }

  auto destruct() -> void {
    terminate();
  }

  auto initialize() -> bool {
    terminate();
    if(!self.fullScreen && !self.context) return false;

    auto monitor = Video::monitor(self.monitor);
    _monitorX = monitor.x;
    _monitorY = monitor.y;
    _monitorWidth = monitor.width;
    _monitorHeight = monitor.height;

    if(self.fullScreen) {
      _context = _window = CreateWindowEx(WS_EX_TOPMOST, L"VideoGDI_Window", L"", WS_VISIBLE | WS_POPUP,
        _monitorX, _monitorY, _monitorWidth, _monitorHeight,
        nullptr, nullptr, GetModuleHandle(0), nullptr);
    } else {
      _context = (HWND)self.context;
    }

    _width = 0;
    _height = 0;
    return _ready = true;
  }

  auto terminate() -> void {
    _ready = false;
    if(_buffer) { delete[] _buffer; _buffer = nullptr; }
    if(_bitmap) { DeleteObject(_bitmap); _bitmap = nullptr; }
    if(_dc) { DeleteDC(_dc); _dc = nullptr; }
    if(_window) { DestroyWindow(_window); _window = nullptr; }
    _context = nullptr;
  }

  bool _ready = false;

  s32 _monitorX = 0;
  s32 _monitorY = 0;
  s32 _monitorWidth = 0;
  s32 _monitorHeight = 0;

  u32* _buffer = nullptr;
  u32 _width = 0;
  u32 _height = 0;

  HWND _window = nullptr;
  HWND _context = nullptr;
  HBITMAP _bitmap = nullptr;
  HDC _dc = nullptr;
  BITMAPINFO _info = {};
};
