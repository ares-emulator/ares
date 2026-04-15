#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#include <librashader/librashader_ld.h>

#include <array>
#include <thread>

static LRESULT CALLBACK d3d12device_windowprocedure(HWND hwnd, UINT msg,
                                                    WPARAM wparam,
                                                    LPARAM lparam) {
  if(msg == WM_SYSKEYDOWN && wparam == VK_F4)
    return false;
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

template <typename T> static auto d3d12device_release(T *&resource) -> void {
  if(resource) {
    resource->Release();
    resource = nullptr;
  }
}

struct d3d12device : VideoDriver {
  d3d12device(Video &super) : VideoDriver(super) { construct(); }
  ~d3d12device() { destruct(); }

  auto create() -> bool override { return initialize(); }

  auto driver() -> string override { return "Direct3D 12"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasExclusive() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }
  auto hasForceSRGB() -> bool override { return true; }
  auto hasThreadedRenderer() -> bool override { return true; }
  auto hasNativeFullScreen() -> bool override { return true; }
  auto hasFlush() -> bool override { return true; }
  auto hasFormats() -> std::vector<string> override {
    return {"ARGB24", "ARGB30"};
  }
  auto hasShader() -> bool override { return true; }

  auto setFullScreen(bool fullScreen) -> bool override {
    this->fullScreen = fullScreen;
    return initialize();
  }
  auto setMonitor(string monitor) -> bool override {
    if(!fullScreen)
      return true;
    return initialize();
  }
  auto setExclusive(bool exclusive) -> bool override {
    if(!fullScreen)
      return true;
    return initialize();
  }
  auto setContext(uintptr context) -> bool override {
    this->context = context;
    return initialize();
  }

  auto setBlocking(bool blocking) -> bool override {
    _blockingEnabled.store(blocking, std::memory_order_relaxed);
    updatePresentMode();
    return true;
  }

  auto setForceSRGB(bool forceSRGB) -> bool override {
    if(_forceSRGB == forceSRGB)
      return true;

    _forceSRGB = forceSRGB;

    lock_guard<std::mutex> lock(_renderMutex);
    applySwapChainColorSpace();
    return true;
  }

  auto setThreadedRenderer(bool threadedRenderer) -> bool override {
    if(_threaded.load(std::memory_order_relaxed) == threadedRenderer)
      return true;
    _threaded.store(threadedRenderer, std::memory_order_relaxed);
    if(!_ready)
      return true;

    if(_threaded.load(std::memory_order_relaxed)) {
      startRenderThread();
    } else {
      stopRenderThread(true);
    }
    updatePresentMode();

    return true;
  }

  auto setNativeFullScreen(bool nativeFullScreen) -> bool override {
    if(fullScreen)
      return initialize();
    return true;
  }

  auto setFlush(bool flush) -> bool override {
    this->flush = flush;
    return true;
  }

  auto setFormat(string format) -> bool override {
    if(!updateSourceFormat(format))
      return false;

    lock_guard<std::mutex> lock(_renderMutex);
    if(_ready && _sourceWidth && _sourceHeight) {
      return resizeSource(_sourceWidth, _sourceHeight);
    }

    return true;
  }

  auto setShader(string shader) -> bool override {
    this->shader = shader;
    return updateShader();
  }

  auto refreshRateHint(double refreshRate) -> void override {
    if(_refreshRateHint == refreshRate)
      return;
    _refreshRateHint = refreshRate;
    updatePresentMode();
  }

  auto focused() -> bool override {
    if(fullScreen && _exclusive)
      return true;
    auto focused = GetFocus();
    return _context == focused || IsChild(_context, focused);
  }

  auto clear() -> void override {
    if(_buffer.size())
      memory::fill(_buffer.data(), _buffer.size() * sizeof(u32));
    if(!_ready)
      return;

    u32 width = 0;
    u32 height = 0;
    size(width, height);
    if(!(width && height))
      return;

    if(_sourceWidth && _sourceHeight) {
      output(width, height);
      return;
    }

    renderFrame(width, height, 0, 0, nullptr);
  }

  auto size(u32& width, u32& height) -> void override {
    if(!_context) {
      width = 0;
      height = 0;
      return;
    }

    RECT rectangle{};
    GetClientRect(_context, &rectangle);

    width = rectangle.right - rectangle.left;
    height = rectangle.bottom - rectangle.top;
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(!_ready)
      return false;
    if(!width || !height)
      return false;

    if(width != _sourceWidth || height != _sourceHeight) {
      _buffer.resize(width * height);
      if(_buffer.size())
        memory::fill(_buffer.data(), _buffer.size() * sizeof(u32));

      lock_guard<std::mutex> lock(_renderMutex);
      if(!resizeSource(width, height))
        return false;
    }

    pitch = _sourceWidth * sizeof(u32);
    data = _buffer.data();
    return true;
  }

  auto release() -> void override {}

  auto output(u32 width, u32 height) -> void override {
    if(_lost.exchange(false)) {
      if(!initialize()) {
        _lost.store(true);
        return;
      }
    }

    if(_threaded.load(std::memory_order_relaxed)) {
      submitThreadedFrame(width, height);
      return;
    }

    renderFrame(width, height, _sourceWidth, _sourceHeight, _buffer.data());
  }

private:
  static constexpr u32 FrameCount = 2;

  struct Vertex {
    float position[4];
    float uv[2];
  };

  auto construct() -> void {
    WNDCLASS windowClass{};
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    windowClass.hInstance = GetModuleHandle(0);
    windowClass.lpfnWndProc = d3d12device_windowprocedure;
    windowClass.lpszClassName = L"d3d12device_window";
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&windowClass);

    _threaded.store(threadedRenderer, std::memory_order_relaxed);
    _blockingEnabled.store(blocking, std::memory_order_relaxed);
    _presentSyncInterval.store(blocking ? 1 : 0, std::memory_order_relaxed);
  }

  auto queryCurrentRefreshRateHz() -> double {
    if(!_context)
      return 0.0;

    auto monitor = MonitorFromWindow(_context, MONITOR_DEFAULTTONEAREST);
    if(!monitor)
      return 0.0;

    MONITORINFOEX monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if(!GetMonitorInfo(monitor, &monitorInfo))
      return 0.0;

    DEVMODE displayMode{};
    displayMode.dmSize = sizeof(displayMode);
    if(!EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS,
                             &displayMode)) {
      return 0.0;
    }

    if(displayMode.dmDisplayFrequency <= 1)
      return 0.0;

    return (double)displayMode.dmDisplayFrequency;
  }

  auto computePresentSyncInterval() -> UINT {
    if(!_blockingEnabled.load(std::memory_order_relaxed))
      return 0;

    if(_refreshRateHint > 0.0) {
      auto refreshRate = queryCurrentRefreshRateHz();
      if(refreshRate > 0.0) {
        auto ratio = refreshRate / _refreshRateHint;
        if(ratio < 1.0)
          ratio = 1.0;

        auto syncInterval = (UINT)(ratio + 0.5);
        if(syncInterval > 4)
          syncInterval = 4;
        return syncInterval;
      }
    }

    return 1;
  }

  auto applySwapChainColorSpace() -> void {
    if(!_swapChain)
      return;

    DXGI_COLOR_SPACE_TYPE targetColorSpace =
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    if(!_forceSRGB) {
      IDXGIOutput *output = nullptr;
      if(SUCCEEDED(_swapChain->GetContainingOutput(&output)) && output) {
        IDXGIOutput6 *output6 = nullptr;
        if(SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(&output6))) &&
            output6) {
          DXGI_OUTPUT_DESC1 outputDesc{};
          if(SUCCEEDED(output6->GetDesc1(&outputDesc))) {
            targetColorSpace = outputDesc.ColorSpace;
          }
          d3d12device_release(output6);
        }
        d3d12device_release(output);
      }
    }

    UINT support = 0;
    if(FAILED(
            _swapChain->CheckColorSpaceSupport(targetColorSpace, &support)) ||
        !(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
      targetColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
      if(FAILED(
              _swapChain->CheckColorSpaceSupport(targetColorSpace, &support)) ||
          !(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
        return;
      }
    }

    _swapChain->SetColorSpace1(targetColorSpace);
  }

  auto destruct() -> void { terminate(); }

  auto initialize() -> bool {
    auto fail = [&]() -> bool {
      terminate();
      return false;
    };

    terminate();
    _lost.store(false);
    _threaded.store(threadedRenderer, std::memory_order_relaxed);
    _blockingEnabled.store(blocking, std::memory_order_relaxed);
    _forceSRGB = forceSRGB;
    if(!updateSourceFormat(format)) {
      updateSourceFormat("ARGB24");
    }
    if(!fullScreen && !context)
      return fail();

    auto monitor = Video::monitor(this->monitor);
    _monitorX = monitor.x;
    _monitorY = monitor.y;
    _monitorWidth = monitor.width;
    _monitorHeight = monitor.height;

    _exclusive = fullScreen && exclusive && !nativeFullScreen;

    if(fullScreen) {
      _context = _window = CreateWindowEx(
          WS_EX_TOPMOST, L"d3d12device_window", L"", WS_VISIBLE | WS_POPUP,
          _monitorX, _monitorY, _monitorWidth, _monitorHeight, nullptr, nullptr,
          GetModuleHandle(0), nullptr);
    } else {
      _context = (HWND)context;
    }

    if(!_context)
      return fail();

    RECT rectangle{};
    GetClientRect(_context, &rectangle);
    _windowWidth = rectangle.right - rectangle.left;
    _windowHeight = rectangle.bottom - rectangle.top;
    if(!_windowWidth)
      _windowWidth = 1;
    if(!_windowHeight)
      _windowHeight = 1;

    if(!createFactory())
      return fail();
    if(!createDevice())
      return fail();
    if(!createQueueAndSwapChain())
      return fail();
    if(!createRenderTargets())
      return fail();
    if(!createCommandObjects())
      return fail();
    if(!createDescriptorHeaps())
      return fail();
    if(!createPipeline())
      return fail();
    if(!createVertexBuffer())
      return fail();

    _libra = librashader_load_instance();
    if(!_libra.instance_loaded) {
      print("Direct3D 12: Failed to load librashader: shaders will be "
            "disabled\n");
    }

    updateShader();

    _ready = true;
    updatePresentMode();
    if(_threaded.load(std::memory_order_relaxed))
      startRenderThread();
    return true;
  }

  auto terminate() -> void {
    _ready = false;
    _lost.store(false);
    stopRenderThread(true);
    lock_guard<std::mutex> renderLock(_renderMutex);

    if(_swapChain) {
      _swapChain->SetFullscreenState(FALSE, nullptr);
    }

    waitForGpu();

    if(_filterChain) {
      _libra.d3d12_filter_chain_free(&_filterChain);
      _filterChain = nullptr;
    }
    if(_preset) {
      _libra.preset_free(&_preset);
      _preset = nullptr;
    }

    _buffer.clear();
    _buffer.shrink_to_fit();

    d3d12device_release(_shaderTarget);
    _shaderTargetState = D3D12_RESOURCE_STATE_COMMON;
    _shaderTargetRtvValid = false;
    _shaderWidth = 0;
    _shaderHeight = 0;

    d3d12device_release(_sourceUpload);
    d3d12device_release(_sourceTexture);
    _sourceWidth = 0;
    _sourceHeight = 0;
    resetSrvDescriptorCache();

    d3d12device_release(_vertexBuffer);
    d3d12device_release(_pipelineState);
    d3d12device_release(_rootSignature);

    d3d12device_release(_srvHeap);

    for(auto &target : _renderTargets) {
      d3d12device_release(target);
    }
    d3d12device_release(_rtvHeap);

    d3d12device_release(_commandList);
    for(auto &allocator : _commandAllocators) {
      d3d12device_release(allocator);
    }

    d3d12device_release(_fence);
    if(_fenceEvent) {
      CloseHandle(_fenceEvent);
      _fenceEvent = nullptr;
    }

    d3d12device_release(_swapChain);
    d3d12device_release(_swapChain2);
    d3d12device_release(_queue);
    d3d12device_release(_device);
    d3d12device_release(_factory);

    if(_window) {
      DestroyWindow(_window);
      _window = nullptr;
    }

    _context = nullptr;
    _frameLatencyWaitableObject = nullptr;
    _frameIndex = 0;
    _fenceValue = 0;
    _fenceValues = {};
  }

  auto resetSrvDescriptorCache() -> void {
    _srvCacheFilterSrcRes = nullptr;
    _srvCacheFilterSrcFmt = DXGI_FORMAT_UNKNOWN;
    _srvCacheBlitRes = nullptr;
    _srvCacheBlitFmt = DXGI_FORMAT_UNKNOWN;
  }

#include <ruby/video/direct3d12/d3d12builders.cpp>
#include <ruby/video/direct3d12/d3d12resources.cpp>
#include <ruby/video/direct3d12/d3d12thread.cpp>
#include <ruby/video/direct3d12/d3d12present.cpp>

  auto updatePresentMode() -> void {
    auto syncInterval = computePresentSyncInterval();
    _presentSyncInterval.store(syncInterval, std::memory_order_relaxed);

    lock_guard<std::mutex> lock(_renderMutex);
    if(!_swapChain2)
      return;
    bool blockingEnabled = _blockingEnabled.load(std::memory_order_relaxed);
    bool threadedEnabled = _threaded.load(std::memory_order_relaxed);
    u32 maxLatency = blockingEnabled ? 1 : (threadedEnabled ? 2 : 1);
    _swapChain2->SetMaximumFrameLatency(maxLatency);
  }

  bool _ready = false;

  HWND _window = nullptr;
  HWND _context = nullptr;

  s32 _monitorX = 0;
  s32 _monitorY = 0;
  s32 _monitorWidth = 0;
  s32 _monitorHeight = 0;
  bool _exclusive = false;

  u32 _windowWidth = 1;
  u32 _windowHeight = 1;

  IDXGIFactory4 *_factory = nullptr;
  ID3D12Device *_device = nullptr;
  ID3D12CommandQueue *_queue = nullptr;
  IDXGISwapChain3 *_swapChain = nullptr;
  IDXGISwapChain2 *_swapChain2 = nullptr;
  HANDLE _frameLatencyWaitableObject = nullptr;

  ID3D12DescriptorHeap *_rtvHeap = nullptr;
  UINT _rtvDescriptorSize = 0;
  std::array<ID3D12Resource *, FrameCount> _renderTargets = {nullptr, nullptr};

  std::array<ID3D12CommandAllocator *, FrameCount> _commandAllocators = {
      nullptr, nullptr};
  ID3D12GraphicsCommandList *_commandList = nullptr;

  ID3D12Fence *_fence = nullptr;
  HANDLE _fenceEvent = nullptr;
  std::array<u64, FrameCount> _fenceValues = {};
  u64 _fenceValue = 0;
  u32 _frameIndex = 0;

  bool _allowTearing = false;
  bool _supportsRootSignature11 = false;
  std::atomic<bool> _lost{false};
  std::atomic<bool> _threaded{true};
  std::atomic<bool> _blockingEnabled{true};
  std::atomic<UINT> _presentSyncInterval{1};
  bool _forceSRGB = false;
  double _refreshRateHint = 0.0;

  std::thread _renderThread;
  std::mutex _threadMutex;
  std::condition_variable _threadCV;
  std::condition_variable _threadDoneCV;
  bool _threadExit = false;
  bool _threadPending = false;
  bool _threadBusy = false;
  std::vector<u32> _threadFrameBuffer;
  u32 _threadSourceWidth = 0;
  u32 _threadSourceHeight = 0;
  u32 _threadOutputWidth = 0;
  u32 _threadOutputHeight = 0;

  std::mutex _renderMutex;

  ID3D12DescriptorHeap *_srvHeap = nullptr;
  UINT _srvDescriptorSize = 0;
  ID3D12Resource *_srvCacheFilterSrcRes = nullptr;
  DXGI_FORMAT _srvCacheFilterSrcFmt = DXGI_FORMAT_UNKNOWN;
  ID3D12Resource *_srvCacheBlitRes = nullptr;
  DXGI_FORMAT _srvCacheBlitFmt = DXGI_FORMAT_UNKNOWN;

  ID3D12RootSignature *_rootSignature = nullptr;
  ID3D12PipelineState *_pipelineState = nullptr;

  ID3D12Resource *_vertexBuffer = nullptr;
  D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};

  ID3D12Resource *_sourceTexture = nullptr;
  ID3D12Resource *_sourceUpload = nullptr;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT _sourceFootprint = {};
  D3D12_RESOURCE_STATES _sourceState = D3D12_RESOURCE_STATE_COPY_DEST;
  DXGI_FORMAT _sourceFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
  u32 _sourceWidth = 0;
  u32 _sourceHeight = 0;
  std::vector<u32> _buffer;

  ID3D12Resource *_shaderTarget = nullptr;
  D3D12_RESOURCE_STATES _shaderTargetState = D3D12_RESOURCE_STATE_COMMON;
  bool _shaderTargetRtvValid = false;
  DXGI_FORMAT _shaderTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
  u32 _shaderWidth = 0;
  u32 _shaderHeight = 0;

  libra_instance_t _libra = {};
  libra_shader_preset_t _preset = nullptr;
  libra_d3d12_filter_chain_t _filterChain = nullptr;
  u32 _frameCount = 0;
};
