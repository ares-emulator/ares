#undef interface
#define interface struct
#include <d3d9.h>
#undef interface

static LRESULT CALLBACK VideoDirect3D9_WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if(msg == WM_SYSKEYDOWN && wparam == VK_F4) return false;
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

struct VideoDirect3D : VideoDriver {
  VideoDirect3D& self = *this;
  VideoDirect3D(Video& super) : VideoDriver(super) { construct(); }
  ~VideoDirect3D() { destruct(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "Direct3D 9.0"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasExclusive() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasShader() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override { return initialize(); }
  auto setMonitor(string monitor) -> bool override { return initialize(); }
  auto setExclusive(bool exclusive) -> bool override { return initialize(); }
  auto setContext(uintptr context) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { return true; }
  auto setShader(string shader) -> bool override { return updateFilter(); }

  auto focused() -> bool override {
    if(self.fullScreen && self.exclusive) return true;
    auto focused = GetFocus();
    return _context == focused || IsChild(_context, focused);
  }

  auto clear() -> void override {
    if(_lost && !recover()) return;

    D3DSURFACE_DESC surfaceDescription;
    _texture->GetLevelDesc(0, &surfaceDescription);
    _texture->GetSurfaceLevel(0, &_surface);

    if(_surface) {
      D3DLOCKED_RECT lockedRectangle;
      _surface->LockRect(&lockedRectangle, 0, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
      memory::fill(lockedRectangle.pBits, lockedRectangle.Pitch * surfaceDescription.Height);
      _surface->UnlockRect();
      _surface->Release();
      _surface = nullptr;
    }

    //clear primary display and all backbuffers
    for(u32 n : range(3)) {
      _device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0x00, 0x00, 0x00), 1.0f, 0);
      _device->Present(0, 0, 0, 0);
    }
  }

  auto size(u32& width, u32& height) -> void override {
    if(_lost && !recover()) return;

    RECT rectangle;
    GetClientRect(_context, &rectangle);

    width = rectangle.right - rectangle.left;
    height = rectangle.bottom - rectangle.top;

    //if output size changed, driver must be re-initialized.
    //failure to do so causes scaling issues on some video drivers.
    if(width != _windowWidth || height != _windowHeight) initialize();
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(_lost && !recover()) return false;

    u32 windowWidth, windowHeight;
    size(windowWidth, windowHeight);

    if(width != _inputWidth || height != _inputHeight) {
      resize(_inputWidth = width, _inputHeight = height);
    }

    D3DSURFACE_DESC surfaceDescription;
    _texture->GetLevelDesc(0, &surfaceDescription);
    _texture->GetSurfaceLevel(0, &_surface);

    D3DLOCKED_RECT lockedRectangle;
    _surface->LockRect(&lockedRectangle, 0, D3DLOCK_NOSYSLOCK | D3DLOCK_DISCARD);
    pitch = lockedRectangle.Pitch;
    return data = (u32*)lockedRectangle.pBits;
  }

  auto release() -> void override {
    _surface->UnlockRect();
    _surface->Release();
    _surface = nullptr;
  }

  auto output(u32 width, u32 height) -> void override {
    if(_lost && !recover()) return;

    if(!width) width = _windowWidth;
    if(!height) height = _windowHeight;

    _device->BeginScene();
    //center output within window
    u32 x = (_windowWidth - width) / 2;
    u32 y = (_windowHeight - height) / 2;
    setVertex(0, 0, _inputWidth, _inputHeight, _textureWidth, _textureHeight, x, y, width, height);
    _device->SetTexture(0, _texture);
    _device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    _device->EndScene();

    if(self.blocking) {
      D3DRASTER_STATUS status;
      while(true) {  //wait for a previous vblank to finish, if necessary
        _device->GetRasterStatus(0, &status);
        if(!status.InVBlank) break;
      }
      while(true) {  //wait for next vblank to begin
        _device->GetRasterStatus(0, &status);
        if(status.InVBlank) break;
      }
    }

    if(_device->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) _lost = true;
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
    windowClass.lpszClassName = L"VideoDirect3D9_Window";
    windowClass.lpszMenuName = 0;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&windowClass);
  }

  auto destruct() -> void {
    terminate();
  }

  auto recover() -> bool {
    if(!_device) return false;

    if(_lost) {
      if(_vertexBuffer) { _vertexBuffer->Release(); _vertexBuffer = nullptr; }
      if(_surface) { _surface->Release(); _surface = nullptr; }
      if(_texture) { _texture->Release(); _texture = nullptr; }
      if(_device->Reset(&_presentation) != D3D_OK) return false;
    }
    _lost = false;

    _device->SetDialogBoxMode(false);

    _device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    _device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    _device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    _device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    _device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    _device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    _device->SetRenderState(D3DRS_LIGHTING, false);
    _device->SetRenderState(D3DRS_ZENABLE, false);
    _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    _device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

    _device->SetVertexShader(nullptr);
    _device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    _device->CreateVertexBuffer(sizeof(Vertex) * 4, _vertexUsage, D3DFVF_XYZRHW | D3DFVF_TEX1,
      (D3DPOOL)_vertexPool, &_vertexBuffer, nullptr);
    _textureWidth = 0;
    _textureHeight = 0;
    resize(_inputWidth = 256, _inputHeight = 256);
    updateFilter();
    clear();
    return true;
  }

  auto resize(u32 width, u32 height) -> void {
    if(_textureWidth >= width && _textureHeight >= height) return;

    _textureWidth = bit::round(max(width, _textureWidth));
    _textureHeight = bit::round(max(height, _textureHeight));

    if(_capabilities.MaxTextureWidth < _textureWidth || _capabilities.MaxTextureWidth < _textureHeight) return;

    if(_texture) _texture->Release();
    _device->CreateTexture(_textureWidth, _textureHeight, 1, _textureUsage, D3DFMT_X8R8G8B8,
      (D3DPOOL)_texturePool, &_texture, nullptr);
  }

  auto updateFilter() -> bool {
    if(!_device) return false;
    if(_lost && !recover()) return false;

    auto filter = self.shader == "Blur" ? D3DTEXF_LINEAR : D3DTEXF_POINT;
    _device->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
    _device->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);
    return true;
  }

  //(x,y) screen coordinates, in pixels
  //(u,v) texture coordinates, betweeen 0.0 (top, left) to 1.0 (bottom, right)
  auto setVertex(u32 px, u32 py, u32 pw, u32 ph, u32 tw, u32 th, u32 x, u32 y, u32 w, u32 h) -> void {
    Vertex vertex[4];
    vertex[0].x = vertex[2].x = (f64)(x     - 0.5);
    vertex[1].x = vertex[3].x = (f64)(x + w - 0.5);
    vertex[0].y = vertex[1].y = (f64)(y     - 0.5);
    vertex[2].y = vertex[3].y = (f64)(y + h - 0.5);

    //Z-buffer and RHW are unused for 2D blit, set to normal values
    vertex[0].z = vertex[1].z = vertex[2].z = vertex[3].z = 0.0;
    vertex[0].rhw = vertex[1].rhw = vertex[2].rhw = vertex[3].rhw = 1.0;

    f64 rw = (f64)w / (f64)pw * (f64)tw;
    f64 rh = (f64)h / (f64)ph * (f64)th;
    vertex[0].u = vertex[2].u = (f64)(px    ) / rw;
    vertex[1].u = vertex[3].u = (f64)(px + w) / rw;
    vertex[0].v = vertex[1].v = (f64)(py    ) / rh;
    vertex[2].v = vertex[3].v = (f64)(py + h) / rh;

    LPDIRECT3DVERTEXBUFFER9* vertexPointer = nullptr;
    _vertexBuffer->Lock(0, sizeof(Vertex) * 4, (void**)&vertexPointer, 0);
    memory::copy<Vertex>(vertexPointer, vertex, 4);
    _vertexBuffer->Unlock();

    _device->SetStreamSource(0, _vertexBuffer, 0, sizeof(Vertex));
  }

  auto initialize() -> bool {
    terminate();
    if(!self.fullScreen && !self.context) return false;

    auto monitor = Video::monitor(self.monitor);
    _monitorX = monitor.x;
    _monitorY = monitor.y;
    _monitorWidth = monitor.width;
    _monitorHeight = monitor.height;

    _exclusive = self.exclusive && self.fullScreen;

    //Direct3D exclusive mode targets the primary monitor only
    if(_exclusive) {
      POINT point{0, 0};  //the primary monitor always starts at (0,0)
      HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
      MONITORINFOEX info{};
      info.cbSize = sizeof(MONITORINFOEX);
      GetMonitorInfo(monitor, &info);
      _monitorX = info.rcMonitor.left;
      _monitorY = info.rcMonitor.top;
      _monitorWidth = info.rcMonitor.right - info.rcMonitor.left;
      _monitorHeight = info.rcMonitor.bottom - info.rcMonitor.top;
    }

    if(self.fullScreen) {
      _context = _window = CreateWindowEx(WS_EX_TOPMOST, L"VideoDirect3D9_Window", L"", WS_VISIBLE | WS_POPUP,
        _monitorX, _monitorY, _monitorWidth, _monitorHeight,
        nullptr, nullptr, GetModuleHandle(0), nullptr);
    } else {
      _context = (HWND)self.context;
    }

    RECT rectangle;
    GetClientRect(_context, &rectangle);
    _windowWidth = rectangle.right - rectangle.left;
    _windowHeight = rectangle.bottom - rectangle.top;

    _instance = Direct3DCreate9(D3D_SDK_VERSION);
    if(!_instance) return false;

    memory::fill(&_presentation, sizeof(_presentation));
    _presentation.Flags = D3DPRESENTFLAG_VIDEO;
    _presentation.SwapEffect = D3DSWAPEFFECT_DISCARD;
    _presentation.BackBufferCount = 1;
    _presentation.MultiSampleType = D3DMULTISAMPLE_NONE;
    _presentation.MultiSampleQuality = 0;
    _presentation.EnableAutoDepthStencil = false;
    _presentation.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    _presentation.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    _presentation.hDeviceWindow = _context;
    _presentation.Windowed = !_exclusive;
    _presentation.BackBufferFormat = _exclusive ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;
    _presentation.BackBufferWidth = _exclusive ? _monitorWidth : 0;
    _presentation.BackBufferHeight = _exclusive ? _monitorHeight : 0;
    _presentation.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    if(_instance->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, _context,
      D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &_presentation, &_device) != D3D_OK) {
      return false;
    }

    _device->GetDeviceCaps(&_capabilities);
    if(_capabilities.Caps2 & D3DCAPS2_DYNAMICTEXTURES) {
      _textureUsage = D3DUSAGE_DYNAMIC;
      _texturePool = D3DPOOL_DEFAULT;
      _vertexUsage = D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
      _vertexPool = D3DPOOL_DEFAULT;
    } else {
      _textureUsage = 0;
      _texturePool = D3DPOOL_MANAGED;
      _vertexUsage = D3DUSAGE_WRITEONLY;
      _vertexPool = D3DPOOL_MANAGED;
    }

    _lost = false;
    return _ready = recover();
  }

  auto terminate() -> void {
    _ready = false;
    if(_vertexBuffer) { _vertexBuffer->Release(); _vertexBuffer = nullptr; }
    if(_surface) { _surface->Release(); _surface = nullptr; }
    if(_texture) { _texture->Release(); _texture = nullptr; }
    if(_device) { _device->Release(); _device = nullptr; }
    if(_instance) { _instance->Release(); _instance = nullptr; }
    if(_window) { DestroyWindow(_window); _window = nullptr; }
    _context = nullptr;
  }

  struct Vertex {
    float x, y, z, rhw;  //screen coordinates
    float u, v;          //texture coordinates
  };

  bool _ready = false;

  HWND _window = nullptr;
  HWND _context = nullptr;
  LPDIRECT3D9 _instance = nullptr;
  LPDIRECT3DDEVICE9 _device = nullptr;
  LPDIRECT3DVERTEXBUFFER9 _vertexBuffer = nullptr;
  D3DPRESENT_PARAMETERS _presentation = {};
  D3DCAPS9 _capabilities = {};
  LPDIRECT3DTEXTURE9 _texture = nullptr;
  LPDIRECT3DSURFACE9 _surface = nullptr;

  bool _exclusive = false;
  bool _lost = true;
  u32 _windowWidth;
  u32 _windowHeight;
  u32 _textureWidth;
  u32 _textureHeight;
  s32 _monitorX;
  s32 _monitorY;
  s32 _monitorWidth;
  s32 _monitorHeight;
  u32 _inputWidth;
  u32 _inputHeight;

  u32 _textureUsage;
  u32 _texturePool;
  u32 _vertexUsage;
  u32 _vertexPool;
};
