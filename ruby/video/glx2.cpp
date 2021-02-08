//Xorg/GLX OpenGL 2.0 driver

//note: this is a fallback driver for use when OpenGL 3.2 is not available.
//see glx.cpp for comments on how this driver operates (they are very similar.)

#if defined(DISPLAY_XORG)
  #include <GL/gl.h>
  #include <GL/glx.h>
  #ifndef glGetProcAddress
    #define glGetProcAddress(name) (*glXGetProcAddress)((const GLubyte*)(name))
  #endif
#elif defined(DISPLAY_QUARTZ)
  #include <OpenGL/gl3.h>
#elif defined(DISPLAY_WINDOWS)
  #include <GL/gl.h>
  #include <GL/glext.h>
  #ifndef glGetProcAddress
    #define glGetProcAddress(name) wglGetProcAddress(name)
  #endif
#else
  #error "ruby::OpenGL2: unsupported platform"
#endif

struct VideoGLX2 : VideoDriver {
  VideoGLX2& self = *this;
  VideoGLX2(Video& super) : VideoDriver(super) { construct(); }
  ~VideoGLX2() { destruct(); }

  auto create() -> bool override {
    VideoDriver::exclusive = true;
    VideoDriver::format = "ARGB24";
    return initialize();
  }

  auto driver() -> string override { return "OpenGL 2.0"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasFlush() -> bool override { return true; }
  auto hasShader() -> bool override { return true; }

  auto hasFormats() -> vector<string> override {
    if(_depth == 30) return {"ARGB30", "ARGB24"};
    if(_depth == 24) return {"ARGB24"};
    return {"ARGB24"};  //fallback
  }

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
    if(glXSwapInterval) glXSwapInterval(blocking);
    releaseContext();
    return true;
  }

  auto setFormat(string format) -> bool override {
    if(format == "ARGB24") {
      _glFormat = GL_UNSIGNED_INT_8_8_8_8_REV;
      return initialize();
    }

    if(format == "ARGB30") {
      _glFormat = GL_UNSIGNED_INT_2_10_10_10_REV;
      return initialize();
    }

    return false;
  }

  auto setShader(string shader) -> bool override {
    return true;
  }

  auto focused() -> bool override {
    return true;
  }

  auto clear() -> void override {
    acquireContext();
    memory::fill<u32>(_glBuffer, _glWidth * _glHeight);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    if(_isDoubleBuffered) glXSwapBuffers(_display, _glXWindow);
    releaseContext();
  }

  auto size(u32& width, u32& height) -> void override {
    if(self.fullScreen) {
      width = _monitorWidth;
      height = _monitorHeight;
    } else {
      XWindowAttributes parent;
      XGetWindowAttributes(_display, _parent, &parent);
      width = parent.width;
      height = parent.height;
    }
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(width != _width || height != _height) resize(width, height);
    pitch = _glWidth * sizeof(u32);
    return data = _glBuffer;
  }

  auto release() -> void override {
  }

  auto output(u32 width, u32 height) -> void override {
    acquireContext();
    XWindowAttributes window;
    XGetWindowAttributes(_display, _window, &window);

    XWindowAttributes parent;
    XGetWindowAttributes(_display, _parent, &parent);

    if(window.width != parent.width || window.height != parent.height) {
      XResizeWindow(_display, _window, parent.width, parent.height);
    }

    u32 viewportX = 0;
    u32 viewportY = 0;
    u32 viewportWidth = parent.width;
    u32 viewportHeight = parent.height;

    if(self.fullScreen) {
      viewportX = _monitorX;
      viewportY = _monitorY;
      viewportWidth = _monitorWidth;
      viewportHeight = _monitorHeight;
    }

    if(!width) width = viewportWidth;
    if(!height) height = viewportHeight;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, self.shader == "Blur" ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, self.shader == "Blur" ? GL_LINEAR : GL_NEAREST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //vertex coordinates range from (0,0) to (1,1) for the entire desktop (all monitors)
    glOrtho(0, 1, 0, 1, -1.0, 1.0);
    //set the viewport to the entire desktop (all monitors)
    glViewport(0, 0, parent.width, parent.height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPixelStorei(GL_UNPACK_ROW_LENGTH, _glWidth);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_BGRA, _glFormat, _glBuffer);

    //normalize texture coordinates and adjust for NPOT textures
    f64 w = (f64)_width / (f64)_glWidth;
    f64 h = (f64)_height / (f64)_glHeight;

    //size of the active monitor
    f64 mw = (f64)viewportWidth / (f64)parent.width;
    f64 mh = (f64)viewportHeight / (f64)parent.height;

    //offset of the active monitor
    f64 mx = (f64)viewportX / (f64)parent.width;
    f64 my = (f64)viewportY / (f64)parent.height;

    //size of the render area
    f64 vw = (f64)width / (f64)parent.width;
    f64 vh = (f64)height / (f64)parent.height;

    //center the render area within the active monitor
    f64 vl = mx + (mw - vw) / 2;
    f64 vt = my + (mh - vh) / 2;
    f64 vr = vl + vw;
    f64 vb = vt + vh;

    //OpenGL places (0,0) at the bottom left; convert our (0,0) at the top left to this form:
    vt = 1.0 - vt;
    vb = 1.0 - vb;

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0); glVertex3f(vl, vt, 0);
    glTexCoord2f(w, 0); glVertex3f(vr, vt, 0);
    glTexCoord2f(0, h); glVertex3f(vl, vb, 0);
    glTexCoord2f(w, h); glVertex3f(vr, vb, 0);
    glEnd();
    glFlush();

    if(_isDoubleBuffered) glXSwapBuffers(_display, _glXWindow);
    if(self.flush) glFinish();
    releaseContext();
  }

  auto poll() -> void override {
    while(XPending(_display)) {
      XEvent event;
      XNextEvent(_display, &event);
      if(event.type == Expose) {
        XWindowAttributes attributes;
        XGetWindowAttributes(_display, _window, &attributes);
        super.doUpdate(attributes.width, attributes.height);
      }
    }
  }

private:
  auto construct() -> void {
    _display = XOpenDisplay(nullptr);
    _screen = DefaultScreen(_display);

    XWindowAttributes attributes{};
    XGetWindowAttributes(_display, RootWindow(_display, _screen), &attributes);
    _depth = attributes.depth;
  }

  auto destruct() -> void {
    terminate();
    XCloseDisplay(_display);
  }

  auto acquireContext() -> void {
    if(!_glXContext) return;
    while(!glXMakeCurrent(_display, _glXWindow, _glXContext)) spinloop();
  }

  auto releaseContext() -> void {
    if(!_glXContext) return;
    while(!glXMakeCurrent(_display, 0, nullptr)) spinloop();
  }

  auto initialize() -> bool {
    terminate();
    if(!self.fullScreen && !self.context) return false;

    s32 versionMajor = 0, versionMinor = 0;
    glXQueryVersion(_display, &versionMajor, &versionMinor);
    if(versionMajor < 1 || (versionMajor == 1 && versionMinor < 2)) return false;

    s32 redDepth   = self.format == "RGB30" ? 10 : 8;
    s32 greenDepth = self.format == "RGB30" ? 10 : 8;
    s32 blueDepth  = self.format == "RGB30" ? 10 : 8;

    s32 attributeList[] = {
      GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_DOUBLEBUFFER, True,
      GLX_RED_SIZE, redDepth,
      GLX_GREEN_SIZE, greenDepth,
      GLX_BLUE_SIZE, blueDepth,
      None
    };

    s32 fbCount = 0;
    auto fbConfig = glXChooseFBConfig(_display, _screen, attributeList, &fbCount);
    if(fbCount == 0) return false;

    auto visual = glXGetVisualFromFBConfig(_display, fbConfig[0]);

    _parent = self.fullScreen ? RootWindow(_display, visual->screen) : (Window)self.context;
    XWindowAttributes windowAttributes;
    XGetWindowAttributes(_display, _parent, &windowAttributes);

    auto monitor = Video::monitor(self.monitor);
    _monitorX = monitor.x;
    _monitorY = monitor.y;
    _monitorWidth = monitor.width;
    _monitorHeight = monitor.height;

    _colormap = XCreateColormap(_display, RootWindow(_display, visual->screen), visual->visual, AllocNone);
    XSetWindowAttributes attributes{};
    attributes.border_pixel = 0;
    attributes.colormap = _colormap;
    attributes.override_redirect = self.fullScreen;
    _window = XCreateWindow(_display, _parent,
      0, 0, windowAttributes.width, windowAttributes.height,
      0, visual->depth, InputOutput, visual->visual,
      CWBorderPixel | CWColormap | CWOverrideRedirect, &attributes);
    XSelectInput(_display, _window, ExposureMask);
    XSetWindowBackground(_display, _window, 0);
    XMapWindow(_display, _window);
    XFlush(_display);

    while(XPending(_display)) {
      XEvent event;
      XNextEvent(_display, &event);
    }

    _glXContext = glXCreateContext(_display, visual, 0, GL_TRUE);
    glXMakeCurrent(_display, _glXWindow = _window, _glXContext);

    if(!glXSwapInterval) glXSwapInterval = (int (*)(int))glGetProcAddress("glXSwapIntervalMESA");
    if(!glXSwapInterval) glXSwapInterval = (int (*)(int))glGetProcAddress("glXSwapIntervalSGI");

    if(glXSwapInterval) glXSwapInterval(self.blocking);

    s32 value = 0;
    glXGetConfig(_display, visual, GLX_DOUBLEBUFFER, &value);
    _isDoubleBuffered = value;
    _isDirect = glXIsDirect(_display, _glXContext);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_POLYGON_SMOOTH);
    glDisable(GL_STENCIL_TEST);

    glEnable(GL_DITHER);
    glEnable(GL_TEXTURE_2D);
    releaseContext();

    resize(256, 256);
    return _ready = true;
  }

  auto terminate() -> void {
    acquireContext();
    _ready = false;

    if(_glTexture) {
      glDeleteTextures(1, &_glTexture);
      _glTexture = 0;
    }

    if(_glBuffer) {
      delete[] _glBuffer;
      _glBuffer = nullptr;
    }

    _glWidth = 0;
    _glHeight = 0;

    if(_glXContext) {
      glXDestroyContext(_display, _glXContext);
      _glXContext = nullptr;
    }

    if(_window) {
      XUnmapWindow(_display, _window);
      _window = 0;
    }

    if(_colormap) {
      XFreeColormap(_display, _colormap);
      _colormap = 0;
    }
  }

  auto resize(u32 width, u32 height) -> void {
    acquireContext();
    _width = width;
    _height = height;

    if(_glTexture == 0) glGenTextures(1, &_glTexture);
    _glWidth = max(_glWidth, width);
    _glHeight = max(_glHeight, height);
    delete[] _glBuffer;
    _glBuffer = new u32[_glWidth * _glHeight]();

    glBindTexture(GL_TEXTURE_2D, _glTexture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, _glWidth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _glWidth, _glHeight, 0, GL_BGRA, _glFormat, _glBuffer);
    releaseContext();
  }

  bool _ready = false;
  bool blur = false;

  Display* _display = nullptr;
  u32 _monitorX = 0;
  u32 _monitorY = 0;
  u32 _monitorWidth = 0;
  u32 _monitorHeight = 0;
  s32 _screen = 0;
  u32 _depth = 24;  //depth of the default root window
  Window _parent = 0;
  Window _window = 0;
  Colormap _colormap = 0;
  GLXContext _glXContext = nullptr;
  GLXWindow _glXWindow = 0;

  bool _isDoubleBuffered = false;
  bool _isDirect = false;

  u32 _width = 256;
  u32 _height = 256;

  GLuint _glTexture = 0;
  u32* _glBuffer = nullptr;
  u32 _glWidth = 0;
  u32 _glHeight = 0;
  u32 _glFormat = GL_UNSIGNED_INT_8_8_8_8_REV;

  auto (*glXSwapInterval)(int) -> int = nullptr;
};
