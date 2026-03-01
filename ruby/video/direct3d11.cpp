
#include "direct3d11/d3d11device.hpp"

typedef std::unique_ptr<D3D11Device> PD3D11Device;

static LRESULT CALLBACK VideoDirect3D11_WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if(msg == WM_SYSKEYDOWN && wparam == VK_F4) return false;
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

struct VideoDirect3D11 : VideoDriver {
  VideoDirect3D11& self = *this;
  VideoDirect3D11(Video& super) : VideoDriver(super) { construct(); }
  ~VideoDirect3D11() { terminate(); }

  auto create() -> bool override { return initialize(); }
  auto driver() -> string override { return "Direct3D 11.1"; }
 
  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasShader() -> bool override { return false; }

  auto setFullScreen(bool fullScreen) -> bool override { return initialize(); }
  auto setMonitor(string monitor) -> bool override { return initialize(); }
  auto setContext(uintptr context) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { _blocking = blocking; return initialize(); }
  auto setShader(string shader) -> bool override { return updateFilter(); }

  auto focused() -> bool override {
    if(self.fullScreen && self.exclusive) return true;
    auto focused = GetFocus();
    return _context == focused || IsChild(_context, focused);
  }

  auto clear() -> void override {
    if(_device) _device->clearRenderTarget(true);
  }

  auto size(u32& width, u32& height) -> void override {
    if(self.fullScreen) {
      width = _monitorWidth;
      height = _monitorHeight;
    } else {
      RECT rectangle;
      GetClientRect(_context, &rectangle);
      width = rectangle.right - rectangle.left;
      height = rectangle.bottom - rectangle.top;
    }
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(!_device || width == 0 || height == 0) return false;

    if(!(_device->updateTextureAndShaderResource(width, height))) return false;

    pitch = _device->getMappedResource().RowPitch;
    return data = (u32*)(_device->getMappedResource().pData);
  }

  auto output(u32 width, u32 height) -> void override {
    if (!_device || width == 0 || height == 0) return;

    u32 windowWidth = 0, windowHeight = 0;
    size(windowWidth, windowHeight);
    _device->render(width, height, windowWidth, windowHeight);
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
    windowClass.lpfnWndProc = VideoDirect3D11_WindowProcedure;
    windowClass.lpszClassName = L"VideoDirect3D11_Window";
    windowClass.lpszMenuName = 0;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&windowClass);

    _device = std::make_unique<D3D11Device>();
  }

  auto updateFilter() -> bool {
    if(!_device) return false;
   
    //acquireContext();
    _device->setShader(self.shader);
    //releaseContext();
    return true;
  }
    
  auto initialize() -> bool {
    terminate();
    if(!self.fullScreen && !self.context) return false;

    auto monitor = Video::monitor(self.monitor);
    _monitorWidth = monitor.width;
    _monitorHeight = monitor.height;
    if(self.fullScreen) {
      _context = _window = CreateWindowEx(WS_EX_NOACTIVATE, L"VideoDirect3D11_Window", L"", WS_VISIBLE | WS_POPUP | WS_DISABLED,
        monitor.x, monitor.y, _monitorWidth, _monitorHeight,
        (HWND)self.context, nullptr, GetModuleHandle(0), nullptr);
    } else {
      _context = (HWND)self.context;
    }

    return _device->initialize(_context, _blocking);
  }
  
  auto terminate() -> void {
    if(_device) { _device->shutdown(); }
    if(_window) { DestroyWindow(_window); _window = nullptr; }
    _context = nullptr;
  }

  PD3D11Device _device = nullptr;
  HWND _window = nullptr;
  HWND _context = nullptr;
  s32 _monitorWidth = 0;
  s32 _monitorHeight = 0;
  bool _blocking = false;
};
