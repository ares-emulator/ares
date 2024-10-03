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

struct VideoMetal : VideoDriver, Metal {
  VideoMetal& self = *this;
  VideoMetal(Video& super) : VideoDriver(super) {}
  ~VideoMetal() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "Metal"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return !_nativeFullScreen; }
  auto hasContext() -> bool override { return true; }
  auto hasFlush() -> bool override { return true; }
  auto hasBlocking() -> bool override {
    if (@available(macOS 10.15.4, *)) {
      return true;
    } else {
      return false;
    }
  }
  auto hasForceSRGB() -> bool override { return true; }
  auto hasThreadedRenderer() -> bool override { return true; }
  auto hasNativeFullScreen() -> bool override { return true; }
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
    return true;
  }
  
  auto setThreadedRenderer(bool threadedRenderer) -> bool override {
    _threaded = threadedRenderer;
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

  auto setFlush(bool flush) -> bool override {
    _flush = flush;
    return true;
  }
  
  auto refreshRateHint(double refreshRate) -> void override {
    if (refreshRate == _refreshRateHint) return;
    _refreshRateHint = refreshRate;
    updatePresentInterval();
  }
  
  auto isVRRSupported() -> bool {
    if (@available(macOS 12.0, *)) {
      NSTimeInterval minInterval = view.window.screen.minimumRefreshInterval;
      NSTimeInterval maxInterval = view.window.screen.maximumRefreshInterval;
      _vrrIsSupported = minInterval != maxInterval;
      return _vrrIsSupported;
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
          NSLog(@"Refresh rate hint changed to %lf", _refreshRateHint);
          averagePresentDuration = _presentInterval;
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
    
    if(file::exists(pathname)) {
      if (auto error = _libra.preset_create(pathname.data(), &_preset)) {
        print(string{"Metal: Failed to load shader: ", pathname, "\n"});
        _libra.error_print(error);
        return false;
      }
      
      if (auto error = _libra.mtl_filter_chain_create(&_preset, _commandQueue, nil, &_filterChain)) {
        print(string{"Metal: Failed to create filter chain for: ", pathname, "\n"});
        _libra.error_print(error);
        return false;
      };
    } else {
      return false;
    }
    return true;
  }

  auto focused() -> bool override {
    return true;
  }

  auto clear() -> void override {
    dispatch_sync(_renderQueue, ^{
      id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
      MTLRenderPassDescriptor *drawableRenderPassDescriptor = view.currentRenderPassDescriptor;
      id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:drawableRenderPassDescriptor];
      [renderEncoder endEncoding];
      id<CAMetalDrawable> drawable = view.currentDrawable;
      [commandBuffer presentDrawable:drawable];
      [view draw];
      [commandBuffer commit];
    });
  }

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
      
      bytesPerRow = sourceWidth * sizeof(u32);
      if (bytesPerRow < 16) bytesPerRow = 16;
        
      for (int i = 0; i < kMaxSourceBuffersInFlight; i++) {
        if (sourceWidth < 1 || sourceHeight < 1) {
          _sourceTextures[i] = nullptr;
          continue;
        }
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor new];
        textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
        textureDescriptor.width = sourceWidth;
        textureDescriptor.height = sourceHeight;
        textureDescriptor.usage = MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
        
        _sourceTextures[i] = [_device newTextureWithDescriptor:textureDescriptor];
      }
      
    }
    pitch = sourceWidth * sizeof(u32);
    return data = buffer;
  }

  auto release() -> void override {}
  
  auto resizeOutputBuffers(u32 width, u32 height) {
    NSLog(@"Resizing output buffers to %i, %i", width, height);
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
    
    _outputX = (_viewWidth - width) / 2;
    _outputY = (_viewHeight - height) / 2;
  }

  auto output(u32 width, u32 height) -> void override {
    /// Synchronously copy the current framebuffer to a Metal texture, then call into the render dispatch queue
    /// either synchronously or asynchronously depending on whether blocking is on and VRR is supported.

    if (depth >= kMaxSourceBuffersInFlight) {
      //if we are running very behind, drop this frame
      return;
    }
    
    //can we do this outside of the output function?
    //currently no, because in theory framebuffer size can change during runtime
    if (width != outputWidth || height != outputHeight) {
      resizeOutputBuffers(width, height);
    }
    
    @autoreleasepool {
      
      frameCount++;
      
      auto index = frameCount % kMaxSourceBuffersInFlight;
      
      auto sourceTexture = _sourceTextures[index];
      
      [sourceTexture replaceRegion:MTLRegionMake2D(0, 0, sourceWidth, sourceHeight) mipmapLevel:0 withBytes:buffer bytesPerRow:bytesPerRow];
      
      if (@available(macOS 10.15.4, *)) {
        depth++;
      }
      
      /// Only block with `dispatch_sync` if blocking enabled and VRR not supported, or if the threaded renderer
      /// is explicitly disabled. if VRR is supported, we should try to not _literally_ block, because we'll be making a best
      /// effort to synchronize to the guest and host refresh rate at the same time. It's easier to do that if we have
      /// assurances that we won't block the emulation thread in the worst case system conditions.
      if ((_blocking && !_vrrIsSupported) || !_threaded) {
        dispatch_sync(_renderQueue, ^{
          outputHelper(width, height, sourceTexture);
        });
      } else {
        dispatch_async(_renderQueue, ^{
          outputHelper(width, height, sourceTexture);
        });
      }
    }
  }

private:
  auto outputHelper(u32 width, u32 height, id<MTLTexture> sourceTexture) -> void {
    /// Uses two render passes (plus librashader's render passes). The first render pass samples the source texture,
    /// consisting of the pixel buffer from the emulator, onto a texture the same size as our eventual output,
    /// `_renderTargetTexture`. Then it calls into librashader, which performs postprocessing onto the same
    /// output texture. Then for the second render pass here, we composite the output texture within ares's viewport.
    /// We defer this second pass because it is performed directly onto the drawable texture, which we want to delay
    /// acquiring for as long as possible.
    
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
        
        [renderEncoder setRenderPipelineState:_renderToTextureRenderPipeline];
        
        [renderEncoder setViewport:(MTLViewport){0, 0, (double)width, (double)height, -1.0, 1.0}];
        
        [renderEncoder setVertexBuffer:_vertexBuffer
                                offset:0
                               atIndex:0];
        
        [renderEncoder setVertexBytes:&_viewportSize
                               length:sizeof(_viewportSize)
                              atIndex:MetalVertexInputIndexViewportSize];
        
        [renderEncoder setFragmentTexture:sourceTexture atIndex:0];
        
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        
        [renderEncoder endEncoding];
        
        if (_filterChain) {
          _libra.mtl_filter_chain_frame(&_filterChain, commandBuffer, frameCount, sourceTexture, _renderTargetTexture, nil, nil, nil);
        }
        
        //this call will block the current thread/queue if a drawable is not yet available
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
          
          if (@available(macOS 10.15.4, *)) {
            [drawable addPresentedHandler:^(id<MTLDrawable> drawable) {
             self.drawableWasPresented(drawable);
             depth--;
             }];
          }
          
          auto targetPresentDuration = determineNextPresentDuration();
          
          if (drawable != nil) {
            if (_blocking) {
              //_blocking is not enabled unless 10.15.4 is available, so ignore availability warnings here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
              [commandBuffer presentDrawable:drawable afterMinimumDuration:targetPresentDuration];
#pragma clang diagnostic pop
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
  
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
  auto drawableWasPresented(id<MTLDrawable> drawable) -> void {
    if (drawable.presentedTime == 0) { return; }
    if (previousPresentedTime <= 0) {
     previousPresentedTime = drawable.presentedTime;
    }
    CFTimeInterval presentationDuration = drawable.presentedTime - previousPresentedTime;
    const double alpha = kPresentIntervalRollingAverageWeight;

    averagePresentDuration = (presentationDuration * alpha) + (averagePresentDuration * (1.0 - alpha));
    previousPresentedTime = drawable.presentedTime;
  }
#pragma clang diagnostic pop
  
  auto determineNextPresentDuration() -> CFTimeInterval {
    /// We use a rolling average of the last few seconds worth of frames to determine if we are running fast or slow. If
    /// we are running ahead, we do nothing special; it's sufficient to present at the prescribed interval and eventually
    /// we will fall behind. When we fall behind, we need to present earlier than the target present interval. The way VRR
    /// works on macOS, we can request an earlier present interval, but we don't always get it. So what we do in this
    /// function is "nudge" the system to display our frame early, but not immediately, in an attempt to correct for running
    /// behind. If in this process we get more than 3 frames behind, we start requesting immediate presents.
    
    CFTimeInterval targetPresentDuration = _presentInterval;
    CFTimeInterval differenceFromTarget = _presentInterval - averagePresentDuration;
    if (-differenceFromTarget >= (_presentInterval * kVRRCorrectiveTolerance)) {
      targetPresentDuration = _presentInterval + (differenceFromTarget * kVRRCorrectiveForce);
    }
    if (depth > kVRRImmediatePresentThreshold) {
      return 0;
    } else {
      return targetPresentDuration;
    }
  }
  
  auto initialize() -> bool {
    terminate();
    if (!self.context) return false;

    auto context = (__bridge NSView*)(void *)self.context;
    auto size = [context frame].size;
    
    NSError *error = nil;
    
    //Put renderer on a separate queue so we can choose whether or not to block the main thread (audio) waiting on a drawable.
    dispatch_queue_attr_t queueAttributes = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, -1);
    _renderQueue = dispatch_queue_create("com.ares.metal-renderer", queueAttributes);
    
    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];
    
    _semaphore = dispatch_semaphore_create(kMaxOutputBuffersInFlight);

    _renderToTextureRenderPassDescriptor = [MTLRenderPassDescriptor new];

    _renderToTextureRenderPassDescriptor.colorAttachments[0].texture = _renderTargetTexture;
    _renderToTextureRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    _renderToTextureRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    _renderToTextureRenderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    ///We compile shaders at runtime so we do not need to add the `xcrun` Metal compiler toolchain to the ares build process.
    ///Metal frame capture does not get along with runtime-compiled shaders in my testing, however. If you are debugging ares
    ///and need GPU captures, run `scripts/macos-metal-debug.sh` and then compile ares in debug mode.

    bool libraryCreated = false;
    
#if defined(BUILD_DEBUG)
    if (@available(macOS 10.13, *)) {
      NSURL *shaderLibURL = [NSURL fileURLWithPath:@"ares.app/Contents/Resources/Shaders/shaders.metallib"];
      _library = [_device newLibraryWithURL: shaderLibURL error:&error];
    }
    if (_library != nil) {
      libraryCreated = true;
    } else {
      NSLog(@"Compiled in debug mode, but debug .metallib not found. If you require Metal debugging, ensure you are on macOS 10.13+ and compile debug Metal shaders with scripts/macos-metal-debug.sh.");
    }
#endif

    if (!libraryCreated) {
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
    view.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;
    
    //driver settings like sync, flush, threaded renderer, etc. will be initialized later

    _libra = librashader_load_instance();
    if (!_libra.instance_loaded) {
      print("Metal: Failed to load librashader: shaders will be disabled\n");
    }
    
    initialized = true;
    setNativeFullScreen(self.nativeFullScreen);
    return _ready = true;
  }

  auto terminate() -> void {
    _ready = false;
    
    _commandQueue = nullptr;
    _library = nullptr;

    _vertexBuffer = nullptr;
    for (int i = 0; i < kMaxSourceBuffersInFlight; i++) {
      _sourceTextures[i] = nullptr;
    }
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
  }

  RubyVideoMetal* view = nullptr;

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
