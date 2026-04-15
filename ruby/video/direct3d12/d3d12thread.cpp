auto updateShader() -> bool {
  lock_guard<std::mutex> renderLock(_renderMutex);

  if(!_device)
    return true;

  if(_filterChain) {
    _libra.d3d12_filter_chain_free(&_filterChain);
    _filterChain = nullptr;
  }
  if(_preset) {
    _libra.preset_free(&_preset);
    _preset = nullptr;
  }
  _frameCount = 0;
  resetSrvDescriptorCache();

  if(!_libra.instance_loaded)
    return true;

  if(shader.imatch("None"))
    return true;

  if(!file::exists(shader))
    return true;

  if(auto error = _libra.preset_create(shader.data(), &_preset)) {
    print("Direct3D 12: Failed to load shader: ", shader, "\n");
    _libra.error_print(error);
    return false;
  }

  filter_chain_d3d12_opt_t options{};
  options.version = LIBRASHADER_CURRENT_VERSION;

  if(auto error = _libra.d3d12_filter_chain_create(&_preset, _device, &options,
                                                    &_filterChain)) {
    print("Direct3D 12: Failed to create filter chain for: ", shader, "\n");
    _libra.error_print(error);
    _libra.preset_free(&_preset);
    return false;
  }

  _preset = nullptr;
  return true;
}

auto submitThreadedFrame(u32 width, u32 height) -> void {
  if(!_ready || !_swapChain || !_context || !_buffer.size())
    return;

  if(!_renderThread.joinable())
    startRenderThread();
  if(!_renderThread.joinable()) {
    renderFrame(width, height, _sourceWidth, _sourceHeight, _buffer.data());
    return;
  }

  {
    lock_guard<std::mutex> lock(_threadMutex);
    _threadSourceWidth = _sourceWidth;
    _threadSourceHeight = _sourceHeight;
    _threadOutputWidth = width;
    _threadOutputHeight = height;
    _threadFrameBuffer.swap(_buffer);
    const size_t needed = (size_t)_threadSourceWidth * _threadSourceHeight;
    if(_buffer.size() != needed)
      _buffer.resize(needed);
    _threadPending = true;
  }

  _threadCV.notify_one();

  if(_blockingEnabled.load(std::memory_order_relaxed)) {
    unique_lock<std::mutex> lock(_threadMutex);
    _threadDoneCV.wait(lock, [&] { return !_threadPending && !_threadBusy; });
  }
}

auto renderThreadLoop() -> void {
  std::vector<u32> frameBuffer;

  while(true) {
    u32 sourceWidth = 0;
    u32 sourceHeight = 0;
    u32 outputWidth = 0;
    u32 outputHeight = 0;

    {
      unique_lock<std::mutex> lock(_threadMutex);
      _threadCV.wait(lock, [&] { return _threadExit || _threadPending; });
      if(_threadExit)
        break;

      frameBuffer.swap(_threadFrameBuffer);
      sourceWidth = _threadSourceWidth;
      sourceHeight = _threadSourceHeight;
      outputWidth = _threadOutputWidth;
      outputHeight = _threadOutputHeight;
      _threadPending = false;
      _threadBusy = true;
    }

    renderFrame(outputWidth, outputHeight, sourceWidth, sourceHeight,
                frameBuffer.data());

    frameBuffer.clear();

    {
      lock_guard<std::mutex> lock(_threadMutex);
      _threadBusy = false;
    }
    _threadDoneCV.notify_all();
  }
}

auto startRenderThread() -> void {
  if(_renderThread.joinable())
    return;

  {
    lock_guard<std::mutex> lock(_threadMutex);
    _threadExit = false;
    _threadPending = false;
    _threadBusy = false;
    _threadFrameBuffer.clear();
  }

  _renderThread = std::thread([this] { renderThreadLoop(); });
}

auto stopRenderThread(bool drain) -> void {
  if(!_renderThread.joinable())
    return;

  {
    unique_lock<std::mutex> lock(_threadMutex);
    if(drain) {
      _threadDoneCV.wait(lock, [&] { return !_threadPending && !_threadBusy; });
    }
    _threadExit = true;
  }

  _threadCV.notify_one();
  _renderThread.join();

  {
    lock_guard<std::mutex> lock(_threadMutex);
    _threadExit = false;
    _threadPending = false;
    _threadBusy = false;
    _threadFrameBuffer.clear();
  }
}
