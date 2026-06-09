auto renderFrame(u32 width, u32 height, u32 sourceWidth, u32 sourceHeight,
                 const u32 *sourceData) -> void {
  if(!_ready || !_swapChain || !_context)
    return;
  bool hasSource = sourceData && sourceWidth && sourceHeight;

  lock_guard<std::mutex> renderLock(_renderMutex);

  u32 windowWidth = 0;
  u32 windowHeight = 0;
  size(windowWidth, windowHeight);
  if(!windowWidth || !windowHeight)
    return;

  if(windowWidth != _windowWidth || windowHeight != _windowHeight) {
    _windowWidth = windowWidth;
    _windowHeight = windowHeight;
    if(!resizeSwapChain())
      return;
  }

  if(!width)
    width = windowWidth;
  if(!height)
    height = windowHeight;

  if(width > windowWidth)
    width = windowWidth;
  if(height > windowHeight)
    height = windowHeight;

  if(hasSource) {
    if(sourceWidth != _sourceWidth || sourceHeight != _sourceHeight ||
        !_sourceTexture || !_sourceUpload) {
      if(!resizeSource(sourceWidth, sourceHeight))
        return;
    }
  }

  if(!beginFrame())
    return;
  ID3D12Resource *sampledResource = nullptr;
  if(hasSource) {
    if(!uploadSourceTexture(sourceData, sourceWidth, sourceHeight)) {
      endFrame(false);
      return;
    }
    sampledResource = _sourceTexture;

    if(_filterChain) {
      if(!ensureShaderTarget(width, height)) {
        endFrame(false);
        return;
      }

      D3D12_SHADER_RESOURCE_VIEW_DESC filterInputSrvDesc{};
      filterInputSrvDesc.Format = _sourceFormat;
      filterInputSrvDesc.Shader4ComponentMapping =
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      filterInputSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      filterInputSrvDesc.Texture2D.MostDetailedMip = 0;
      filterInputSrvDesc.Texture2D.MipLevels = 1;

      auto filterInputSrvHandle =
          _srvHeap->GetCPUDescriptorHandleForHeapStart();
      if(_sourceTexture != _srvCacheFilterSrcRes ||
          _sourceFormat != _srvCacheFilterSrcFmt) {
        _device->CreateShaderResourceView(_sourceTexture, &filterInputSrvDesc,
                                          filterInputSrvHandle);
        _srvCacheFilterSrcRes = _sourceTexture;
        _srvCacheFilterSrcFmt = _sourceFormat;
      }

      transitionResource(_shaderTarget, _shaderTargetState,
                         D3D12_RESOURCE_STATE_RENDER_TARGET);
      _shaderTargetState = D3D12_RESOURCE_STATE_RENDER_TARGET;

      libra_image_d3d12_t input{};
      input.image_type = LIBRA_D3D12_IMAGE_TYPE_SOURCE_IMAGE;
      input.handle.source.descriptor = filterInputSrvHandle;
      input.handle.source.resource = _sourceTexture;

      libra_image_d3d12_t output{};
      output.image_type = LIBRA_D3D12_IMAGE_TYPE_RESOURCE;
      output.handle.resource = _shaderTarget;

      if(auto error = _libra.d3d12_filter_chain_frame(
              &_filterChain, _commandList, _frameCount++, input, output,
              nullptr, nullptr, nullptr)) {
        _libra.error_print(error);
        sampledResource = _sourceTexture;
      } else {
        sampledResource = _shaderTarget;
      }

      transitionResource(_shaderTarget, _shaderTargetState,
                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
      _shaderTargetState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
  }

  transitionResource(_renderTargets[_frameIndex], D3D12_RESOURCE_STATE_PRESENT,
                     D3D12_RESOURCE_STATE_RENDER_TARGET);

  auto rtvStart = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
  D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvStart;
  rtv.ptr += _frameIndex * _rtvDescriptorSize;

  _commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

  float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  _commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

  D3D12_VIEWPORT viewport{};
  viewport.TopLeftX = (float)((windowWidth - width) / 2);
  viewport.TopLeftY = (float)((windowHeight - height) / 2);
  viewport.Width = (float)width;
  viewport.Height = (float)height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;

  D3D12_RECT scissor{};
  scissor.left = (LONG)viewport.TopLeftX;
  scissor.top = (LONG)viewport.TopLeftY;
  scissor.right = scissor.left + (LONG)width;
  scissor.bottom = scissor.top + (LONG)height;

  _commandList->RSSetViewports(1, &viewport);
  _commandList->RSSetScissorRects(1, &scissor);

  if(hasSource) {
    const DXGI_FORMAT blitFmt =
        sampledResource == _shaderTarget ? _shaderTargetFormat : _sourceFormat;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = blitFmt;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    auto blitSrvCpu = _srvHeap->GetCPUDescriptorHandleForHeapStart();
    blitSrvCpu.ptr += _srvDescriptorSize;
    if(sampledResource != _srvCacheBlitRes || blitFmt != _srvCacheBlitFmt) {
      _device->CreateShaderResourceView(sampledResource, &srvDesc, blitSrvCpu);
      _srvCacheBlitRes = sampledResource;
      _srvCacheBlitFmt = blitFmt;
    }

    auto blitSrvGpu = _srvHeap->GetGPUDescriptorHandleForHeapStart();
    blitSrvGpu.ptr += _srvDescriptorSize;

    ID3D12DescriptorHeap *descriptorHeaps[] = {_srvHeap};
    _commandList->SetDescriptorHeaps(1, descriptorHeaps);
    _commandList->SetGraphicsRootSignature(_rootSignature);
    _commandList->SetPipelineState(_pipelineState);
    _commandList->SetGraphicsRootDescriptorTable(0, blitSrvGpu);

    _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    _commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    _commandList->DrawInstanced(4, 1, 0, 0);
  }

  transitionResource(_renderTargets[_frameIndex],
                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                     D3D12_RESOURCE_STATE_PRESENT);
  endFrame(true);
}

auto beginFrame() -> bool {
  bool blockingEnabled = _blockingEnabled.load(std::memory_order_relaxed);
  if(blockingEnabled && _frameLatencyWaitableObject) {
    DWORD waitResult = WaitForSingleObject(_frameLatencyWaitableObject, 100);
    if(waitResult == WAIT_FAILED) {
      markLost();
      return false;
    }
    if(waitResult == WAIT_TIMEOUT)
      return false;
    if(waitResult != WAIT_OBJECT_0 && waitResult != WAIT_ABANDONED) {
      markLost();
      return false;
    }
  }

  if(_fence->GetCompletedValue() < _fenceValues[_frameIndex]) {
    auto signalResult =
        _fence->SetEventOnCompletion(_fenceValues[_frameIndex], _fenceEvent);
    if(FAILED(signalResult)) {
      markLost();
      return false;
    }
    DWORD waitResult = WaitForSingleObject(_fenceEvent, INFINITE);
    if(waitResult != WAIT_OBJECT_0 && waitResult != WAIT_ABANDONED) {
      markLost();
      return false;
    }
  }

  HRESULT hr = _commandAllocators[_frameIndex]->Reset();
  if(FAILED(hr)) {
    markLost();
    return false;
  }

  hr = _commandList->Reset(_commandAllocators[_frameIndex], nullptr);
  if(FAILED(hr)) {
    markLost();
    return false;
  }

  return true;
}

auto endFrame(bool present) -> bool {
  HRESULT hr = _commandList->Close();
  if(FAILED(hr)) {
    markLost();
    return false;
  }

  ID3D12CommandList *lists[] = {_commandList};
  _queue->ExecuteCommandLists(1, lists);

  UINT presentFlags = 0;
  if(present) {
    bool blockingEnabled = _blockingEnabled.load(std::memory_order_relaxed);
    bool threadedEnabled = _threaded.load(std::memory_order_relaxed);
    if(!blockingEnabled && _allowTearing && !_exclusive) {
      presentFlags = DXGI_PRESENT_ALLOW_TEARING;
    }
    if(!blockingEnabled && threadedEnabled) {
      presentFlags |= DXGI_PRESENT_DO_NOT_WAIT;
    }

    auto syncInterval = _presentSyncInterval.load(std::memory_order_relaxed);
    auto status = _swapChain->Present(syncInterval, presentFlags);
    if(status != DXGI_ERROR_WAS_STILL_DRAWING &&
        status != DXGI_STATUS_OCCLUDED && FAILED(status)) {
      markLost();
      return false;
    }
  }

  auto signalValue = ++_fenceValue;
  hr = _queue->Signal(_fence, signalValue);
  if(FAILED(hr)) {
    markLost();
    return false;
  }

  _fenceValues[_frameIndex] = signalValue;
  _frameIndex = _swapChain->GetCurrentBackBufferIndex();

  if(flush)
    waitForGpu();
  return true;
}

auto waitForGpu() -> void {
  if(!_queue || !_fence || !_fenceEvent)
    return;

  auto signalValue = ++_fenceValue;
  HRESULT hr = _queue->Signal(_fence, signalValue);
  if(FAILED(hr)) {
    markLost();
    return;
  }
  hr = _fence->SetEventOnCompletion(signalValue, _fenceEvent);
  if(FAILED(hr)) {
    markLost();
    return;
  }
  DWORD waitResult = WaitForSingleObject(_fenceEvent, INFINITE);
  if(waitResult != WAIT_OBJECT_0 && waitResult != WAIT_ABANDONED) {
    markLost();
    return;
  }

  for(u32 index = 0; index < FrameCount; index++) {
    _fenceValues[index] = signalValue;
  }
}

auto resizeSwapChain() -> bool {
  if(!_swapChain || !_device)
    return false;

  waitForGpu();

  for(auto& target : _renderTargets) {
    d3d12device_release(target);
  }

  DXGI_SWAP_CHAIN_DESC desc{};
  HRESULT hr = _swapChain->GetDesc(&desc);
  if(FAILED(hr)) {
    markLost();
    return false;
  }

  auto width = _windowWidth ? _windowWidth : 1;
  auto height = _windowHeight ? _windowHeight : 1;

  hr = _swapChain->ResizeBuffers(FrameCount, width, height,
                                 desc.BufferDesc.Format, desc.Flags);
  if(FAILED(hr)) {
    markLost();
    return false;
  }

  applySwapChainColorSpace();

  _frameIndex = _swapChain->GetCurrentBackBufferIndex();

  d3d12device_release(_rtvHeap);
  return createRenderTargets();
}

auto markLost() -> void { _lost.store(true); }

auto transitionResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES before,
                        D3D12_RESOURCE_STATES after) -> void {
  if(!resource || before == after)
    return;

  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = resource;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  _commandList->ResourceBarrier(1, &barrier);
}
