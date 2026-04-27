auto createFactory() -> bool {
  HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&_factory));
  if(FAILED(hr))
    return false;

  IDXGIFactory5 *factory5 = nullptr;
  if(SUCCEEDED(_factory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
    BOOL allowTearing = FALSE;
    if(SUCCEEDED(factory5->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing,
            sizeof(allowTearing)))) {
      _allowTearing = allowTearing;
    }
    d3d12device_release(factory5);
  }

  return true;
}

auto detectRootSignatureSupport() -> void {
  D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureFeature{};
  rootSignatureFeature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
  if(FAILED(_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
                                         &rootSignatureFeature,
                                         sizeof(rootSignatureFeature)))) {
    _supportsRootSignature11 = false;
  } else {
    _supportsRootSignature11 =
        rootSignatureFeature.HighestVersion >= D3D_ROOT_SIGNATURE_VERSION_1_1;
  }
}

auto createDevice() -> bool {
  if(SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                 IID_PPV_ARGS(&_device)))) {
    detectRootSignatureSupport();
    return true;
  }

  // nullptr picks the default adapter, fall back to enumeration if that fails
  IDXGIAdapter1 *adapter = nullptr;
  for(UINT index = 0;
      _factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND;
      index++) {
    DXGI_ADAPTER_DESC1 desc{};
    adapter->GetDesc1(&desc);
    if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      d3d12device_release(adapter);
      continue;
    }

    if(SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
                                   IID_PPV_ARGS(&_device)))) {
      d3d12device_release(adapter);
      detectRootSignatureSupport();
      return true;
    }

    d3d12device_release(adapter);
  }

  return false;
}

auto createQueueAndSwapChain() -> bool {
  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  HRESULT hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_queue));
  if(FAILED(hr))
    return false;

  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = _windowWidth;
  desc.Height = _windowHeight;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = FrameCount;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.Flags = (_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) |
               DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

  IDXGISwapChain1 *swapChain1 = nullptr;
  hr = _factory->CreateSwapChainForHwnd(_queue, _context, &desc, nullptr,
                                        nullptr, &swapChain1);
  if(FAILED(hr))
    return false;

  _factory->MakeWindowAssociation(_context, DXGI_MWA_NO_ALT_ENTER);  // we handle fullscreen ourselves

  hr = swapChain1->QueryInterface(IID_PPV_ARGS(&_swapChain));
  if(FAILED(hr)) {
    d3d12device_release(swapChain1);
    return false;
  }
  d3d12device_release(swapChain1);

  _swapChain->QueryInterface(IID_PPV_ARGS(&_swapChain2));
  if(_swapChain2) {
    _frameLatencyWaitableObject = _swapChain2->GetFrameLatencyWaitableObject();
  }

  applySwapChainColorSpace();

  _frameIndex = _swapChain->GetCurrentBackBufferIndex();

  if(_exclusive) {
    if(FAILED(_swapChain->SetFullscreenState(TRUE, nullptr))) {
      _exclusive = false;
    }
  }

  return true;
}
