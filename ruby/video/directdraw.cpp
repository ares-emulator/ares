#include <ddraw.h>
#undef interface

static LRESULT CALLBACK VideoDirectDraw7_WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if(msg == WM_SYSKEYDOWN && wparam == VK_F4) return false;
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

struct VideoDirectDraw : VideoDriver {
  VideoDirectDraw& self = *this;
  VideoDirectDraw(Video& super) : VideoDriver(super) { construct(); }
  ~VideoDirectDraw() { destruct(); }

  auto create() -> bool override {
    VideoDriver::shader = "Blur";
    return initialize();
  }

  auto driver() -> string override { return "DirectDraw 7.0"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override {
    return initialize();
  }

  auto setMonitor(string monitor) -> bool override {
    return initialize();
  }

  auto setContext(uintptr context) -> bool override {
    return initialize();
  }

  auto setBlocking(bool blocking) -> bool override {
    return true;
  }

  auto focused() -> bool override {
    if(self.fullScreen && self.exclusive) return true;
    auto focused = GetFocus();
    return _context == focused || IsChild(_context, focused);
  }

  auto clear() -> void override {
    DDBLTFX fx{};
    fx.dwSize = sizeof(DDBLTFX);
    fx.dwFillColor = 0x00000000;
    _screen->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    _raster->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
  }

  auto size(u32& width, u32& height) -> void override {
    RECT rectangle;
    GetClientRect(_context, &rectangle);
    width = rectangle.right - rectangle.left;
    height = rectangle.bottom - rectangle.top;
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(width != _width || height != _height) resize(_width = width, _height = height);
    DDSURFACEDESC2 description = {};
    description.dwSize = sizeof(DDSURFACEDESC2);
    if(_raster->Lock(0, &description, DDLOCK_WAIT, 0) != DD_OK) {
      _raster->Restore();
      if(_raster->Lock(0, &description, DDLOCK_WAIT, 0) != DD_OK) return false;
    }
    pitch = description.lPitch;
    return data = (u32*)description.lpSurface;
  }

  auto release() -> void override {
    _raster->Unlock(0);
  }

  auto output(u32 width, u32 height) -> void override {
    u32 windowWidth, windowHeight;
    size(windowWidth, windowHeight);

    if(self.blocking) while(true) {
      BOOL vblank;
      _interface->GetVerticalBlankStatus(&vblank);
      if(vblank) break;
    }

    RECT source;
    SetRect(&source, 0, 0, _width, _height);

    POINT point{0, 0};
    ClientToScreen(_context, &point);

    RECT target;
    GetClientRect(_context, &target);
    OffsetRect(&target, point.x, point.y);

    target.left += ((s32)windowWidth - (s32)width) / 2;
    target.top += ((s32)windowHeight - (s32)height) / 2;
    target.right = target.left + width;
    target.bottom = target.top + height;

    if(_screen->Blt(&target, _raster, &source, DDBLT_WAIT, 0) == DDERR_SURFACELOST) {
      _screen->Restore();
      _raster->Restore();
    }
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
    windowClass.lpfnWndProc = VideoDirect3D9_WindowProcedure;
    windowClass.lpszClassName = L"VideoDirectDraw7_Window";
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
      _context = _window = CreateWindowEx(WS_EX_TOPMOST, L"VideoDirectDraw7_Window", L"", WS_VISIBLE | WS_POPUP,
        _monitorX, _monitorY, _monitorWidth, _monitorHeight,
        nullptr, nullptr, GetModuleHandle(0), nullptr);
    } else {
      _context = (HWND)self.context;
    }

    LPDIRECTDRAW interface = nullptr;
    DirectDrawCreate(0, &interface, 0);
    interface->QueryInterface(IID_IDirectDraw7, (void**)&_interface);
    interface->Release();

    _interface->SetCooperativeLevel(_context, DDSCL_NORMAL);

    DDSURFACEDESC2 description{};
    description.dwSize = sizeof(DDSURFACEDESC2);
    description.dwFlags = DDSD_CAPS;
    description.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    _interface->CreateSurface(&description, &_screen, 0);

    _interface->CreateClipper(0, &_clipper, 0);
    _clipper->SetHWnd(0, _context);
    _screen->SetClipper(_clipper);

    _raster = nullptr;
    _surfaceWidth = 0;
    _surfaceHeight = 0;
    resize(_width = 256, _height = 256);
    return _ready = true;
  }

  auto terminate() -> void {
    _ready = false;
    if(_clipper) { _clipper->Release(); _clipper = nullptr; }
    if(_raster) { _raster->Release(); _raster = nullptr; }
    if(_screen) { _screen->Release(); _screen = nullptr; }
    if(_interface) { _interface->Release(); _interface = nullptr; }
    if(_window) { DestroyWindow(_window); _window = nullptr; }
    _context = nullptr;
  }

  auto resize(u32 width, u32 height) -> void {
    if(_surfaceWidth >= width && _surfaceHeight >= height) return;

    _surfaceWidth = max(width, _surfaceWidth);
    _surfaceHeight = max(height, _surfaceHeight);

    if(_raster) _raster->Release();

    DDSURFACEDESC2 description{};
    description.dwSize = sizeof(DDSURFACEDESC2);
    _screen->GetSurfaceDesc(&description);
    s32 depth = description.ddpfPixelFormat.dwRGBBitCount;
    if(depth == 32) goto tryNativeSurface;

    memory::fill(&description, sizeof(DDSURFACEDESC2));
    description.dwSize = sizeof(DDSURFACEDESC2);
    description.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    description.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;  //DDSCAPS_SYSTEMMEMORY
    description.dwWidth = _surfaceWidth;
    description.dwHeight = _surfaceHeight;

    description.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    description.ddpfPixelFormat.dwFlags = DDPF_RGB;
    description.ddpfPixelFormat.dwRGBBitCount = 32;
    description.ddpfPixelFormat.dwRBitMask = 0xff0000;
    description.ddpfPixelFormat.dwGBitMask = 0x00ff00;
    description.ddpfPixelFormat.dwBBitMask = 0x0000ff;

    if(_interface->CreateSurface(&description, &_raster, 0) == DD_OK) return clear();

  tryNativeSurface:
    memory::fill(&description, sizeof(DDSURFACEDESC2));
    description.dwSize = sizeof(DDSURFACEDESC2);
    description.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    description.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;  //DDSCAPS_SYSTEMMEMORY
    description.dwWidth = _surfaceWidth;
    description.dwHeight = _surfaceHeight;

    if(_interface->CreateSurface(&description, &_raster, 0) == DD_OK) return clear();
  }

  bool _ready = false;

  s32 _monitorX = 0;
  s32 _monitorY = 0;
  s32 _monitorWidth = 0;
  s32 _monitorHeight = 0;

  u32 _width = 0;
  u32 _height = 0;

  HWND _context = nullptr;
  HWND _window = nullptr;
  LPDIRECTDRAW7 _interface = nullptr;
  LPDIRECTDRAWSURFACE7 _screen = nullptr;
  LPDIRECTDRAWSURFACE7 _raster = nullptr;
  LPDIRECTDRAWCLIPPER _clipper = nullptr;
  u32 _surfaceWidth = 0;
  u32 _surfaceHeight = 0;
};
