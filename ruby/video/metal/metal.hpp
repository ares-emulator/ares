//
//  metal.hpp
//  ares
//
//  Created by jcm on 3/4/24.
//

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

#include "librashader_ld.h"
#include "ShaderTypes.h"

struct Metal;

static const NSUInteger kMaxBuffersInFlight = 3;

struct Metal {
  auto setShader(const string& pathname) -> void;
  auto clear() -> void;
  auto output() -> void;
  auto initialize(const string& shader) -> bool;
  auto terminate() -> void;
  auto refreshRateHint(double refreshRate) -> void;
  
  auto size(u32 width, u32 height) -> void;
  auto release() -> void;
  auto render(u32 sourceWidth, u32 sourceHeight, u32 targetX, u32 targetY, u32 targetWidth, u32 targetHeight) -> void;
  
  u32 *buffer = nullptr;
  
  u32 sourceWidth = 0;
  u32 sourceHeight = 0;
  u32 bytesPerRow = 0;
  
  u32 outputWidth = 0;
  u32 outputHeight = 0;
  double _outputX = 0;
  double _outputY = 0;
  
  CGFloat _viewWidth = 0;
  CGFloat _viewHeight = 0;
  vector_uint2 _viewportSize;
  
  double _presentInterval = .016;
  u32 frameCount = 0;
  double _refreshRateHint = 60;
  
  bool _blocking = false;
  bool _flush = false;
  
  id<MTLDevice> _device;
  id<MTLCommandQueue> _commandQueue;
  id<MTLLibrary> _library;
  dispatch_semaphore_t _semaphore;
  
  id<MTLBuffer> _vertexBuffer;
  id<MTLTexture> _sourceTexture;
  MTLVertexDescriptor *_mtlVertexDescriptor;
  
  MTLRenderPassDescriptor *_renderToTextureRenderPassDescriptor;
  id<MTLRenderPipelineState> _renderToTextureRenderPipeline;
  id<MTLRenderPipelineState> _drawableRenderPipeline;
  id<MTLTexture> _renderTargetTexture;
  
  libra_instance_t _libra;
  libra_shader_preset_t _preset;
  libra_mtl_filter_chain_t _filterChain;
  libra_viewport_t _libraViewport;
  bool initialized = false;
};
