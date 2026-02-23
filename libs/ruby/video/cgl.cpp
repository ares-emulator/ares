#define GL_ALPHA_TEST 0x0bc0
#include "opengl/opengl.hpp"

struct VideoCGL;

@interface RubyVideoCGL : NSOpenGLView {
@public
  VideoCGL* video;
}
-(id) initWith:(VideoCGL*)video pixelFormat:(NSOpenGLPixelFormat*)pixelFormat;
-(void) reshape;
-(BOOL) acceptsFirstResponder;
@end

struct VideoCGL : VideoDriver, OpenGL {
  VideoCGL& self = *this;
  VideoCGL(Video& super) : VideoDriver(super) {}
  ~VideoCGL() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "OpenGL 3.2"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasNativeFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return !_nativeFullScreen; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasForceSRGB() -> bool override { return false; }
  auto hasFlush() -> bool override { return true; }
  auto hasShader() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override {
    // todo: fix/make consistent mouse cursor hide behavior
    
    if (_nativeFullScreen) {
      [view.window toggleFullScreen:nil];
    } else {
      /// This option implements non-idiomatic macOS fullscreen behavior that sets the window frame equal to the selected display's
      /// frame size and hides the cursor. This version of fullscreen is desirable because it allows us to render around the camera
      /// housing on newer Macs (important for bezel-style shaders), has snappier entrance/exit and tabbing behavior, and functions
      /// better with recording and capture software such as OBS.
      if (fullScreen) {
        auto monitor = Video::monitor(self.monitor);
        NSScreen *handle = (__bridge NSScreen *)(void *)monitor.nativeHandle; //eew
        frameBeforeFullScreen = view.window.frame;
        [NSApp setPresentationOptions:(NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar)];
        [view.window setStyleMask:NSWindowStyleMaskBorderless];
        [view.window setFrame:handle.frame display:YES];
        [NSCursor setHiddenUntilMouseMoves:YES];
      } else {
        [NSApp setPresentationOptions:NSApplicationPresentationDefault];
        [view.window setStyleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable)];
        [view.window setFrame:frameBeforeFullScreen display:YES];
      }
      [view.window makeFirstResponder:view];
    }
    return true;
  }
  
  auto setNativeFullScreen(bool nativeFullScreen) -> bool override {
    _nativeFullScreen = nativeFullScreen;
    if (nativeFullScreen) {
      //maximize goes fullscreen
      [view.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
    } else {
      //maximize does not go fullscreen
      [view.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenAuxiliary];
    }
    return true;
  }

  auto setContext(uintptr context) -> bool override {
    return initialize();
  }

  auto setBlocking(bool blocking) -> bool override {
    if(!view) return true;
    acquireContext();
    s32 blocking32 = self.blocking;
    [[view openGLContext] setValues:&blocking32 forParameter:NSOpenGLCPSwapInterval];
    releaseContext();
    return true;
  }

  auto setFlush(bool flush) -> bool override {
    return true;
  }

  auto setShader(string shader) -> bool override {
    acquireContext();
    OpenGL::setShader(shader);
    releaseContext();
    return true;
  }

  auto focused() -> bool override {
    return true;
  }

  auto clear() -> void override {
    acquireContext();
    [view lockFocus];
    OpenGL::clear();
    [[view openGLContext] flushBuffer];
    [view unlockFocus];
    releaseContext();
  }

  auto size(u32& width, u32& height) -> void override {
    auto area = [view convertRectToBacking:[view bounds]];
    width = area.size.width;
    height = area.size.height;
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
    lock_guard<recursive_mutex> lock(mutex);
    acquireContext();
    u32 windowWidth, windowHeight;
    size(windowWidth, windowHeight);

    if([view lockFocusIfCanDraw]) {
      OpenGL::absoluteWidth = width;
      OpenGL::absoluteHeight = height;
      OpenGL::outputX = 0;
      OpenGL::outputY = 0;
      OpenGL::outputWidth = windowWidth;
      OpenGL::outputHeight = windowHeight;
      OpenGL::output();

      [[view openGLContext] flushBuffer];
      if(self.flush) glFinish();
      [view unlockFocus];
    }
    releaseContext();
  }

private:
  auto acquireContext() -> void {
    lock_guard<recursive_mutex> lock(mutex);
    [[view openGLContext] makeCurrentContext];
  }

  auto releaseContext() -> void {
    lock_guard<recursive_mutex> lock(mutex);
    [NSOpenGLContext clearCurrentContext];
  }

  auto initialize() -> bool {
    terminate();
    if(!self.fullScreen && !self.context) return false;

    NSOpenGLPixelFormatAttribute attributeList[] = {
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      NSOpenGLPFAColorSize, 24,
      NSOpenGLPFAAlphaSize, 8,
      NSOpenGLPFADoubleBuffer,
      0
    };

    auto context = (__bridge NSView*)(void *)self.context;
    auto size = [context frame].size;
    auto format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributeList];
    auto openGLContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];

    view = [[RubyVideoCGL alloc] initWith:this pixelFormat:format];
    [view setOpenGLContext:openGLContext];
    [view setFrame:NSMakeRect(0, 0, size.width, size.height)];
    [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [view setWantsBestResolutionOpenGLSurface:YES];
    [context addSubview:view];
    [openGLContext setView:view];
    [openGLContext makeCurrentContext];
    [[view window] makeFirstResponder:view];
    [view lockFocus];

    OpenGL::initialize(self.shader);

    s32 blocking = self.blocking;
    [[view openGLContext] setValues:&blocking forParameter:NSOpenGLCPSwapInterval];

    [view unlockFocus];


    releaseContext();
    clear();
    setNativeFullScreen(_nativeFullScreen);
    return _ready = true;
  }

  auto terminate() -> void {
    acquireContext();
    _ready = false;
    OpenGL::terminate();

    if(view) {
      [view removeFromSuperview];
      view = nil;
    }
  }

  RubyVideoCGL* view = nullptr;

  bool _nativeFullScreen = false;
  NSRect frameBeforeFullScreen = NSMakeRect(0,0,0,0);
  bool _ready = false;
  std::recursive_mutex mutex;
};

@implementation RubyVideoCGL : NSOpenGLView

-(id) initWith:(VideoCGL*)videoPointer pixelFormat:(NSOpenGLPixelFormat*)pixelFormat {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0) pixelFormat:pixelFormat]) {
    video = videoPointer;
  }
  return self;
}

-(void) reshape {
  [super reshape];
}

-(BOOL) acceptsFirstResponder {
  return YES;
}

-(void) keyDown:(NSEvent*)event {
}

-(void) keyUp:(NSEvent*)event {
}

@end
