#include "opengl/opengl.hpp"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092

static LRESULT CALLBACK VideoOpenGL32_WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  if(msg == WM_SYSKEYDOWN && wparam == VK_F4) return false;
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

struct VideoWGL : VideoDriver, OpenGL {
  VideoWGL& self = *this;
  VideoWGL(Video& super) : VideoDriver(super) { construct(); }
  ~VideoWGL() { destruct(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "OpenGL 3.2"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasFlush() -> bool override { return true; }
  auto hasShader() -> bool override { return true; }

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
    acquireContext();
    if(wglSwapInterval) wglSwapInterval(blocking);
    releaseContext();
    return true;
  }

  auto setFlush(bool flush) -> bool override {
    return true;
  }

  auto setShader(string shader) -> bool override {
    acquireContext();
    OpenGL::setShader(self.shader);
    releaseContext();
    return true;
  }

  auto focused() -> bool override {
    if(self.fullScreen && self.exclusive) return true;
    auto focused = GetFocus();
    return _context == focused || IsChild(_context, focused);
  }

  auto clear() -> void override {
    acquireContext();
    OpenGL::clear();
    SwapBuffers(_display);
    releaseContext();
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
    acquireContext();
    OpenGL::size(width, height);
    bool result = OpenGL::lock(data, pitch);
    releaseContext();
    return result;
  }

  auto release() -> void override {
  }

  auto output(u32 width, u32 height) -> void override {
    acquireContext();
    u32 windowWidth, windowHeight;
    size(windowWidth, windowHeight);

    OpenGL::absoluteWidth = width;
    OpenGL::absoluteHeight = height;
    OpenGL::outputX = 0;
    OpenGL::outputY = 0;
    OpenGL::outputWidth = windowWidth;
    OpenGL::outputHeight = windowHeight;
    OpenGL::output();

    SwapBuffers(_display);
    if(self.flush) glFinish();
    releaseContext();
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
    windowClass.lpfnWndProc = VideoOpenGL32_WindowProcedure;
    windowClass.lpszClassName = L"VideoOpenGL32_Window";
    windowClass.lpszMenuName = 0;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&windowClass);
  }

  auto destruct() -> void {
    terminate();
  }

  auto acquireContext() -> void {
    if(!_wglContext) return;
    while(!wglMakeCurrent(_display, _wglContext)) spinloop();
  }

  auto releaseContext() -> void {
    if(!_wglContext) return;
    while(!wglMakeCurrent(_display, nullptr)) spinloop();
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
      _context = _window = CreateWindowEx(WS_EX_NOACTIVATE, L"VideoOpenGL32_Window", L"", WS_VISIBLE | WS_POPUP | WS_DISABLED,
        _monitorX, _monitorY, _monitorWidth, _monitorHeight,
        (HWND)self.context, nullptr, GetModuleHandle(0), nullptr);
    } else {
      _context = (HWND)self.context;
    }

    PIXELFORMATDESCRIPTOR descriptor{};
    descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;

    _display = GetDC(_context);
    GLuint pixelFormat = ChoosePixelFormat(_display, &descriptor);
    SetPixelFormat(_display, pixelFormat, &descriptor);

    _wglContext = wglCreateContext(_display);
    wglMakeCurrent(_display, _wglContext);

    wglCreateContextAttribs = (HGLRC (APIENTRY*)(HDC, HGLRC, const int*))glGetProcAddress("wglCreateContextAttribsARB");
    wglSwapInterval = (BOOL (APIENTRY*)(int))glGetProcAddress("wglSwapIntervalEXT");

    if(wglCreateContextAttribs) {
      s32 attributeList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 2,
        0
      };
      HGLRC context = wglCreateContextAttribs(_display, 0, attributeList);
      if(context) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(_wglContext);
        wglMakeCurrent(_display, _wglContext = context);
      }
    }

    if(wglSwapInterval) wglSwapInterval(self.blocking);
    _ready = OpenGL::initialize(self.shader);
    releaseContext();
    return _ready;
  }

  auto terminate() -> void {
    acquireContext();
    _ready = false;
    OpenGL::terminate();

    if(_wglContext) {
      wglDeleteContext(_wglContext);
      _wglContext = nullptr;
    }

    if(_window) {
      DestroyWindow(_window);
      _window = nullptr;
    }

    _context = nullptr;
  }

  auto (APIENTRY* wglCreateContextAttribs)(HDC, HGLRC, const int*) -> HGLRC = nullptr;
  auto (APIENTRY* wglSwapInterval)(int) -> BOOL = nullptr;

  bool _ready = false;

  s32 _monitorX = 0;
  s32 _monitorY = 0;
  s32 _monitorWidth = 0;
  s32 _monitorHeight = 0;

  HWND _window = nullptr;
  HWND _context = nullptr;
  HDC _display = nullptr;
  HGLRC _wglContext = nullptr;
  HINSTANCE _glWindow = nullptr;
};
