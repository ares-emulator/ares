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

@interface RubyWindowCGL : NSWindow <NSWindowDelegate> {
@public
  VideoCGL* video;
}
-(id) initWith:(VideoCGL*)video;
-(BOOL) canBecomeKeyWindow;
-(BOOL) canBecomeMainWindow;
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
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasFlush() -> bool override { return true; }
  auto hasShader() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override {
    return initialize();
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

    if(self.fullScreen) {
      window = [[RubyWindowCGL alloc] initWith:this];
      [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
      [window toggleFullScreen:nil];
    //[NSApp setPresentationOptions:NSApplicationPresentationFullScreen];
    }

    NSOpenGLPixelFormatAttribute attributeList[] = {
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      NSOpenGLPFAColorSize, 24,
      NSOpenGLPFAAlphaSize, 8,
      NSOpenGLPFADoubleBuffer,
      0
    };

    auto context = self.fullScreen ? [window contentView] : (__bridge NSView*)(void *)self.context;
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

    if(window) {
    //[NSApp setPresentationOptions:NSApplicationPresentationDefault];
      [window toggleFullScreen:nil];
      [window setCollectionBehavior:NSWindowCollectionBehaviorDefault];
      [window close];
      window = nil;
    }
  }

  RubyVideoCGL* view = nullptr;
  RubyWindowCGL* window = nullptr;

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
  video->output(0, 0);
}

-(BOOL) acceptsFirstResponder {
  return YES;
}

-(void) keyDown:(NSEvent*)event {
}

-(void) keyUp:(NSEvent*)event {
}

@end

@implementation RubyWindowCGL : NSWindow

-(id) initWith:(VideoCGL*)videoPointer {
  auto primaryRect = [[[NSScreen screens] objectAtIndex:0] frame];
  if(self = [super initWithContentRect:primaryRect styleMask:0 backing:NSBackingStoreBuffered defer:YES]) {
    video = videoPointer;
    [self setDelegate:self];
    [self setReleasedWhenClosed:NO];
    [self setAcceptsMouseMovedEvents:YES];
    [self setTitle:@""];
    [self makeKeyAndOrderFront:nil];
  }
  return self;
}

-(BOOL) canBecomeKeyWindow {
  return YES;
}

-(BOOL) canBecomeMainWindow {
  return YES;
}

@end
