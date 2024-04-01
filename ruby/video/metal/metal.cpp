//
//  metal.cpp
//  ares
//
//  Created by jcm on 3/4/24.
//

#include "metal.hpp"

struct VideoMetal;

@interface RubyVideoMetal : MTKView <MTKViewDelegate> {
@public
  VideoMetal* video;
}
-(id) initWith:(VideoMetal*)video frame:(NSRect)frame device:(id<MTLDevice>)metalDevice;
-(void) mtkView:(MTKView *)view drawableSizeWillChange:(CGSize) size;
-(BOOL) acceptsFirstResponder;
@end

@interface RubyWindowMetal : NSWindow <NSWindowDelegate> {
@public
  VideoMetal* video;
}
-(id) initWith:(VideoMetal*)video;
-(BOOL) canBecomeKeyWindow;
-(BOOL) canBecomeMainWindow;
@end

struct VideoMetal : VideoDriver, Metal {
  VideoMetal& self = *this;
  VideoMetal(Video& super) : VideoDriver(super) {}
  ~VideoMetal() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "Metal"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return false; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override {
    if (@available(macOS 10.15.4, *)) {
      return !isVRRSupported();
    } else {
      return false;
    }
  }
  auto hasForceSRGB() -> bool override { return true; }
  auto hasFlush() -> bool override { return true; }
  auto hasShader() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override {
    return initialize();
  }

  auto setContext(uintptr context) -> bool override {
    return initialize();
  }

  auto setBlocking(bool blocking) -> bool override {
    _blocking = blocking;
    updatePresentInterval();
    return true;
  }
  
  auto setForceSRGB(bool forceSRGB) -> bool override {
    if (forceSRGB) {
      view.colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    } else {
      view.colorspace = view.window.screen.colorSpace.CGColorSpace;
    }
  }

  auto setFlush(bool flush) -> bool override {
    _flush = flush;
    return true;
  }
  
  auto hintRefreshRate(double refreshRate) -> void {
    _refreshRateHint = refreshRate;
    updatePresentInterval();
  }
  
  auto isVRRSupported() -> bool {
    if (@available(macOS 12.0, *)) {
      NSTimeInterval minInterval = view.window.screen.minimumRefreshInterval;
      NSTimeInterval maxInterval = view.window.screen.maximumRefreshInterval;
      return minInterval != maxInterval;
    } else {
      return false;
    }
  }
  
  auto updatePresentInterval() -> void {
    if (!isVRRSupported()) {
      CGDirectDisplayID displayID = CGMainDisplayID();
      CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(displayID);
      CFTimeInterval refreshRate = CGDisplayModeGetRefreshRate(displayMode);
      _presentInterval = (1.0 / refreshRate);
    } else {
      if (@available(macOS 12.0, *)) {
        CFTimeInterval minimumInterval = view.window.screen.minimumRefreshInterval;
        if (_refreshRateHint != 0) {
          _presentInterval = (1.0 / _refreshRateHint);
        } else {
          _presentInterval = minimumInterval;
        }
      }
    }
  }

  auto setShader(string pathname) -> bool override {
    if (_filterChain != NULL) {
      _libra.mtl_filter_chain_free(&_filterChain);
    }

    if (_preset != NULL) {
      _libra.preset_free(&_preset);
    }

    if (_libra.preset_create(pathname.data(), &_preset) != NULL) {
      print(string{"Metal: Failed to load shader: ", pathname, "\n"});
      return false;
    }
    
    if (_libra.mtl_filter_chain_create(&_preset, _commandQueue, nil, &_filterChain) != NULL) {
      print(string{"Metal: Failed to create filter chain for: ", pathname, "\n"});
      return false;
    };
    return true;
  }

  auto focused() -> bool override {
    return true;
  }

  auto clear() -> void override {}

  auto size(u32& width, u32& height) -> void override {
    if ((_viewWidth == width && _viewHeight == height) && (_viewWidth != 0 && _viewHeight != 0)) { return; }
    auto area = [view convertRectToBacking:[view bounds]];
    width = area.size.width;
    height = area.size.height;
    auto newSize = CGSize();
    newSize.width = width;
    newSize.height = height;
    view.drawableSize = newSize;
    
    _viewWidth = width;
    _viewHeight = height;
    
    _outputX = (width - outputWidth) / 2;
    _outputY = (height - outputHeight) / 2;
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if (sourceWidth != width || sourceHeight != height) {
      
      sourceWidth = width, sourceHeight = height;
      
      if (buffer) {
        delete[] buffer;
        buffer = nullptr;
      }
      
      buffer = new u32[width * height]();
      
      MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor new];
      textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
      textureDescriptor.width = sourceWidth;
      textureDescriptor.height = sourceHeight;
      textureDescriptor.usage = MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
      
      _sourceTexture = [_device newTextureWithDescriptor:textureDescriptor];
      
      bytesPerRow = sourceWidth * sizeof(u32);
      if (bytesPerRow < 16) bytesPerRow = 16;
      
    }
    pitch = sourceWidth * sizeof(u32);
    return data = buffer;
  }

  auto release() -> void override {}
  
  auto resizeOutputBuffers(u32 width, u32 height) {
    outputWidth = width;
    outputHeight = height;
    
    float widthfloat = (float)width;
    float heightfloat = (float)height;
    
    MetalVertex vertices[] =
    {
      // Pixel positions, Texture coordinates
      { {  widthfloat / 2,  -heightfloat / 2 },  { 1.f, 1.f } },
      { { -widthfloat / 2,  -heightfloat / 2 },  { 0.f, 1.f } },
      { { -widthfloat / 2,   heightfloat / 2 },  { 0.f, 0.f } },
      
      { {  widthfloat / 2,  -heightfloat / 2 },  { 1.f, 1.f } },
      { { -widthfloat / 2,   heightfloat / 2 },  { 0.f, 0.f } },
      { {  widthfloat / 2,   heightfloat / 2 },  { 1.f, 0.f } },
    };
    
    _vertexBuffer = [_device newBufferWithBytes:vertices length:sizeof(vertices) options:MTLResourceStorageModeShared];
    
    MTLTextureDescriptor *texDescriptor = [MTLTextureDescriptor new];
    texDescriptor.textureType = MTLTextureType2D;
    texDescriptor.width = width;
    texDescriptor.height = height;
    texDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
    texDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    
    _renderTargetTexture = [_device newTextureWithDescriptor:texDescriptor];
    
    _viewportSize.x = width;
    _viewportSize.y = height;

    _libraViewport.width = (uint32_t) width;
    _libraViewport.height = (uint32_t) height;
    _libraViewport.x = 0;
    _libraViewport.y = 0;
    
    _outputX = (_viewWidth - width) / 2;
    _outputY = (_viewHeight - height) / 2;
  }

  auto output(u32 width, u32 height) -> void override {
    /// Uses two render passes (plus librashader's render passes). The first render pass samples the source texture,
    /// consisting of the pixel buffer from the emulator, onto a texture the same size as our eventual output,
    /// `_renderTargetTexture`. Then it calls into librashader, which performs postprocessing onto the same
    /// output texture. Then for the second render pass here, we composite the output texture within ares's viewport.
    /// We need this last pass because librashader expects the viewport to be the same size as the output texture,
    /// which is not the case for ares.
    
    //can we do this outside of the output function?
    if (width != outputWidth || height != outputHeight) {
      resizeOutputBuffers(width, height);
    }
    
    @autoreleasepool {
      
      dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
      
      id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
      
      if (commandBuffer != nil) {
        __block dispatch_semaphore_t block_sema = _semaphore;
        
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
         dispatch_semaphore_signal(block_sema);
        }];
        
        _renderToTextureRenderPassDescriptor.colorAttachments[0].texture = _renderTargetTexture;
        
        if (_renderToTextureRenderPassDescriptor != nil) {
          
          id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_renderToTextureRenderPassDescriptor];
          
          _renderToTextureRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
          
          [_sourceTexture replaceRegion:MTLRegionMake2D(0, 0, sourceWidth, sourceHeight) mipmapLevel:0 withBytes:buffer bytesPerRow:bytesPerRow];
          
          [renderEncoder setRenderPipelineState:_renderToTextureRenderPipeline];
          
          [renderEncoder setViewport:(MTLViewport){0, 0, (double)width, (double)height, -1.0, 1.0}];
          
          [renderEncoder setVertexBuffer:_vertexBuffer
                                  offset:0
                                 atIndex:0];
          
          [renderEncoder setVertexBytes:&_viewportSize
                                 length:sizeof(_viewportSize)
                                atIndex:MetalVertexInputIndexViewportSize];
          
          [renderEncoder setFragmentTexture:_sourceTexture atIndex:0];
          
          [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
          
          [renderEncoder endEncoding];
          
          if (_filterChain) {
            _libra.mtl_filter_chain_frame(&_filterChain, commandBuffer, frameCount++, _sourceTexture, _libraViewport, _renderTargetTexture, nil, nil);
          }
          
          MTLRenderPassDescriptor *drawableRenderPassDescriptor = view.currentRenderPassDescriptor;
          
          drawableRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
          
          if (drawableRenderPassDescriptor != nil) {
            
            id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:drawableRenderPassDescriptor];
            
            [renderEncoder setRenderPipelineState:_drawableRenderPipeline];
            
            [renderEncoder setViewport:(MTLViewport){_outputX, _outputY, (double)width, (double)height, -1.0, 1.0}];
            
            [renderEncoder setVertexBuffer:_vertexBuffer
                                    offset:0
                                   atIndex:0];
            
            [renderEncoder setVertexBytes:&_viewportSize
                                   length:sizeof(_viewportSize)
                                  atIndex:MetalVertexInputIndexViewportSize];
            
            [renderEncoder setFragmentTexture:_renderTargetTexture atIndex:0];
            
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
            
            [renderEncoder endEncoding];
            
            id<CAMetalDrawable> drawable = view.currentDrawable;
            
            if (drawable != nil) {
              
              if (_blocking) {
                
                [commandBuffer presentDrawable:drawable afterMinimumDuration:_presentInterval];
                
              } else {
                
                [commandBuffer presentDrawable:drawable];
                
              }
              
              [view draw];
              
            }
          }
          
          [commandBuffer commit];
          
          if (_flush) {
            [commandBuffer waitUntilCompleted];
          }
        }
      }
    }
  }

private:
  auto initialize() -> bool {
    terminate();
    if (!self.fullScreen && !self.context) return false;

    auto context = self.fullScreen ? [window contentView] : (__bridge NSView*)(void *)self.context;
    auto size = [context frame].size;
    
    NSError *error = nil;
    
    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];
    
    _semaphore = dispatch_semaphore_create(kMaxBuffersInFlight);

    _renderToTextureRenderPassDescriptor = [MTLRenderPassDescriptor new];

    _renderToTextureRenderPassDescriptor.colorAttachments[0].texture = _renderTargetTexture;
    _renderToTextureRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    _renderToTextureRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    _renderToTextureRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    ///We compile shaders at runtime so we do not need to add the `xcrun` Metal compiler toolchain to the ares build process.
    ///Metal frame capture does not get along with runtime-compiled shaders in my testing, however. If you are debugging ares
    ///and need GPU captures, you should compile shaders with debug symbols offline, then instantiate the shader library by
    ///directly referencing a compiled `.metallib` file, rather than the following instantiation flow. You will also need to alter
    ///the `desktop-ui` Makefile such that it copies the `.metallib` into the bundle, rather than only the `Shaders.metal`
    ///source.

    NSString *bundleResourcePath = [NSBundle mainBundle].resourcePath;
    const string& fileComponent = "/Shaders/Shaders.metal";
    NSString *shaderFilePath = [bundleResourcePath stringByAppendingString: [[NSString new] initWithUTF8String:fileComponent]];
    
    NSString *shaderLibrarySource = [NSString stringWithContentsOfFile:shaderFilePath encoding:NSUTF8StringEncoding error: &error];
    
    if (shaderLibrarySource == nil) {
      NSLog(@"%@",error);
      return false;
    }
    
    _library = [_device newLibraryWithSource: shaderLibrarySource options: [MTLCompileOptions alloc] error:&error];
    
    if (_library == nil) {
      NSLog(@"%@",error);
      return false;
    }
    
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
    
    // Set up pipeline for rendering to the offscreen texture. Reuse the
    // descriptor and change properties that differ.
    pipelineStateDescriptor.label = @"Offscreen Render Pipeline";
    pipelineStateDescriptor.sampleCount = 1;
    pipelineStateDescriptor.vertexFunction = [_library newFunctionWithName:@"vertexShader"];
    pipelineStateDescriptor.fragmentFunction = [_library newFunctionWithName:@"samplingShader"];
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    _renderToTextureRenderPipeline = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    
    if (_renderToTextureRenderPipeline == nil) {
      NSLog(@"%@",error);
      return false;
    }
    
    pipelineStateDescriptor.label = @"Drawable Render Pipeline";
    pipelineStateDescriptor.sampleCount = 1;
    pipelineStateDescriptor.vertexFunction = [_library newFunctionWithName:@"vertexShader"];
    pipelineStateDescriptor.fragmentFunction = [_library newFunctionWithName:@"drawableSamplingShader"];
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    _drawableRenderPipeline = [_device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    
    if (_drawableRenderPipeline == nil) {
      NSLog(@"%@",error);
      return false;
    }
    
    auto frame = NSMakeRect(0, 0, size.width, size.height);
    view = [[RubyVideoMetal alloc] initWith:this frame:frame device:_device];
    [context addSubview:view];
    [[view window] makeFirstResponder:view];
    bool forceSRGB = self.forceSRGB;
    self.setForceSRGB(forceSRGB);
    view.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;

    _commandQueue = [_device newCommandQueue];

    _libra = librashader_load_instance();
    if (!_libra.instance_loaded) {
      print("Metal: Failed to load librashader: shaders will be disabled\n");
    }
    
    setShader(self.shader);
    
    _blocking = self.blocking;
    
    initialized = true;
    return _ready = true;
  }

  auto terminate() -> void {
    _ready = false;
    
    _commandQueue = nullptr;
    _library = nullptr;

    _vertexBuffer = nullptr;
    _sourceTexture = nullptr;
    _mtlVertexDescriptor = nullptr;
    
    _renderToTextureRenderPassDescriptor = nullptr;
    _renderTargetTexture = nullptr;
    _renderToTextureRenderPipeline = nullptr;
    
    _drawableRenderPipeline = nullptr;
    
    if (_filterChain) {
      _libra.mtl_filter_chain_free(&_filterChain);
    }
    _device = nullptr;

    if (view) {
      [view removeFromSuperview];
      view = nil;
    }

    if (window) {
      [window toggleFullScreen:nil];
      [window setCollectionBehavior:NSWindowCollectionBehaviorDefault];
      [window close];
      window = nil;
    }
  }

  RubyVideoMetal* view = nullptr;
  RubyWindowMetal* window = nullptr;

  bool _ready = false;
  std::recursive_mutex mutex;
};

@implementation RubyVideoMetal : MTKView

-(id) initWith:(VideoMetal*)videoPointer frame:(NSRect)frame device:(id<MTLDevice>)metalDevice {
  if (self = [super initWithFrame:frame device:metalDevice]) {
    video = videoPointer;
  }
  self.enableSetNeedsDisplay = NO;
  self.paused = YES;
  
  //below is the delegate path; currently not used but likely will be used in future.
  
  //self.enableSetNeedsDisplay = YES;
  //self.paused = NO;
  //[self setDelegate:self];
  
  return self;
}

-(void) drawInMTKView:(MTKView *)view {
  //currently not used
  //video->alternateDrawPath();
}

-(void) mtkView:(MTKView *)view drawableSizeWillChange:(CGSize) size {
  
}

-(BOOL) acceptsFirstResponder {
  return YES;
}

-(void) keyDown:(NSEvent*)event {
}

-(void) keyUp:(NSEvent*)event {
}

@end

@implementation RubyWindowMetal : NSWindow

-(id) initWith:(VideoMetal*)videoPointer {
  auto primaryRect = [[[NSScreen screens] objectAtIndex:0] frame];
  if (self = [super initWithContentRect:primaryRect styleMask:0 backing:NSBackingStoreBuffered defer:YES]) {
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
