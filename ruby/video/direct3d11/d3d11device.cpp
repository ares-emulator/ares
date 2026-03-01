
auto D3D11Device::initialize(HWND context, bool blocking) -> bool {
  if(!createDeviceAndSwapChain(context, blocking)) return false;
  if(!createRenderTarget()) return false;
  if(!compileShaders()) return false;
  if(!createGeometry()) return false;
  if(!createSamplerState()) return false;

  _libra = librashader_load_instance();
  if(!_libra.instance_loaded) {
    print("D3D11: Failed to load librashader: shaders will be disabled\n");
  }

    return true;
}

auto D3D11Device::shutdown(void) -> void {
  resetRenderTargetView();
  if(_pDeviceContext) { 
    _pDeviceContext->ClearState(); 
    _pDeviceContext->Flush();
  }
  if(_pSwapChain1) _pSwapChain1->SetFullscreenState(false, nullptr);
}

auto D3D11Device::createDeviceAndSwapChain(HWND context, bool blocking) -> bool {
  u32 createFlags = 0;
#if defined(_DEBUG)
  createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
  D3D_FEATURE_LEVEL featureLevel;
  hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags, featureLevels, ARRAYSIZE(featureLevels),
                          D3D11_SDK_VERSION, &_pDevice, &featureLevel, &_pDeviceContext);
  if(FAILED(hr)) {
    print("D3D11: Failed to create device - 0x", hex(hr), "\n");
    return false;
  }

  // Obtain DXGI factory from device
  ComPtr<IDXGIDevice> dxgiDevice;
  _pDevice.As(&dxgiDevice);
  ComPtr<IDXGIAdapter> dxgiAdapter;
  dxgiDevice->GetAdapter(&dxgiAdapter);
  dxgiAdapter->GetParent(IID_PPV_ARGS(&_dxgiFactory2));
  if(_dxgiFactory2) {
    // Check runtime support for allowing tearing (variable refresh / uncapped presents)
    ComPtr<IDXGIFactory6> dxgiFactory6;
    if(SUCCEEDED(_dxgiFactory2.As(&dxgiFactory6))) {
      BOOL allowTearing = FALSE;
      if(SUCCEEDED(dxgiFactory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
        _tearingSupport = (allowTearing == TRUE);
      }
    }
  }
  if(!_tearingSupport) print("D3D11: Warning - allow tearing is not supported, VRR is not available\n");
  _vsyncEnabled = blocking;
      
  // Describe flip-model swap chain
  DXGI_SWAP_CHAIN_DESC1 sd1 = {};
  sd1.Width = 0;
  sd1.Height = 0;
  sd1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd1.Stereo = TRUE;
  sd1.SampleDesc.Count = 1;
  sd1.SampleDesc.Quality = 0;
  sd1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd1.BufferCount = 2;
  sd1.Scaling = DXGI_SCALING_NONE;
  sd1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  sd1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  sd1.Flags = _tearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

  if(_pSwapChain1) _pSwapChain1->Release();
  hr = _dxgiFactory2->CreateSwapChainForHwnd(_pDevice.Get(), context, &sd1, nullptr, nullptr, _pSwapChain1.GetAddressOf());
  if(FAILED(hr)) {
    print("D3D11: Failed to create flip-model swap chain - 0x", hex(hr), "\n");
    return false;
  }
  
  return true;
}

auto D3D11Device::createRenderTarget(void) -> bool {
  ComPtr<ID3D11Texture2D> backBuffer;
  hr = _pSwapChain1->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
  if(FAILED(hr)) {
    print("D3D11: Failed getting back buffer for render target - 0x", hex(hr), "\n");
    return false;
  }

  hr = _pDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, &_pRenderTargetView);
  if(FAILED(hr)) {
    print("D3D11: Failed creating render target view - 0x", hex(hr), "\n");
    return false;
  }
  
 return true;
}

auto D3D11Device::compileShaders(void) -> bool {
  // Simple vertex shader + pixel shader. Vertex shader passes UV; pixel shader samples a texture.
  const char* pVSSrc =
    "struct VSInput { float3 pos : POSITION; float2 uv : TEXCOORD0; };"
    "struct PSInput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };"
    "PSInput main(VSInput vin) { PSInput o; o.pos = float4(vin.pos, 1.0); o.uv = vin.uv; return o; }";

  const char* pPSSrc =
    "Texture2D tex0 : register(t0);"
    "SamplerState samp : register(s0);"
    "struct PSInput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };"
    "float4 main(PSInput IN) : SV_TARGET { return tex0.Sample(samp, IN.uv); }";

  ComPtr<ID3DBlob> pVSBlob, pPSBlob, pErrBlob;
  hr = D3DCompile(pVSSrc, strlen(pVSSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &pVSBlob, &pErrBlob);
  if(FAILED(hr)) {
    if (pErrBlob) OutputDebugStringA((char*)pErrBlob->GetBufferPointer());
    print("D3D11: Vertex shader compile failed - 0x", hex(hr), "\n");
    return false;
  }

  hr = D3DCompile(pPSSrc, strlen(pPSSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &pPSBlob, &pErrBlob);
  if(FAILED(hr)) {
    if (pErrBlob) OutputDebugStringA((char*)pErrBlob->GetBufferPointer());
    print("D3D11: Pixel shader compile failed - 0x", hex(hr), "\n");
    return false;
  }

  hr = _pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);
  if(FAILED(hr)) {
    print("D3D11: Failed to create vertex shader - 0x", hex(hr), "\n");
    return false;
  }

  hr = _pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
  if(FAILED(hr)) {
    print("D3D11: Failed to create pixel shader - 0x", hex(hr), "\n");
    return false;
  }

  // Input layout (position, uv)
  D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, x), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, u), D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  hr = _pDevice->CreateInputLayout(layoutDesc, ARRAYSIZE(layoutDesc), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &_pInputLayout);
  if(FAILED(hr)) {
    print("D3D11: Failed to create input layout - 0x", hex(hr), "\n");
    return false;
  }
  
  return true;
}

auto D3D11Device::createGeometry(void) -> bool {
  Vertex vertices[] =
  {
    // x, y, z,            u, v
    { -1.0f,  1.0f, 0.0f,  0.0f, 0.0f }, // TL
    {  1.0f,  1.0f, 0.0f,  1.0f, 0.0f }, // TR
    {  1.0f, -1.0f, 0.0f,  1.0f, 1.0f }, // BR
    { -1.0f, -1.0f, 0.0f,  0.0f, 1.0f }, // BL
  };

  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  D3D11_BUFFER_DESC vbDesc = {};
  vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vbDesc.Usage = D3D11_USAGE_DEFAULT;
  vbDesc.ByteWidth = sizeof(vertices);
  D3D11_SUBRESOURCE_DATA vbData = {};
  vbData.pSysMem = vertices;
  vbData.SysMemPitch = 0;
  vbData.SysMemSlicePitch = 0;

  hr = _pDevice->CreateBuffer(&vbDesc, &vbData, &_pVertexBuffer);
  if(FAILED(hr)) {
    print("D3D11: Failed to create vertex buffer - 0x", hex(hr), "\n");
    return false;
  }

  D3D11_BUFFER_DESC ibDesc = {};
  ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  ibDesc.Usage = D3D11_USAGE_DEFAULT;
  ibDesc.ByteWidth = sizeof(indices);
  D3D11_SUBRESOURCE_DATA ibData = {};
  ibData.pSysMem = indices;
  ibData.SysMemPitch = 0;
  ibData.SysMemSlicePitch = 0;

  hr = _pDevice->CreateBuffer(&ibDesc, &ibData, &_pIndexBuffer);
  if(FAILED(hr)) {
    print("D3D11: Failed to create index buffer - 0x", hex(hr), "\n");
    return false;
  }

  return true;
}

auto D3D11Device::createSamplerState() -> bool {
  D3D11_SAMPLER_DESC sd = {};
  sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sd.MinLOD = 0;
  sd.MaxLOD = D3D11_FLOAT32_MAX;

  hr = _pDevice->CreateSamplerState(&sd, &_pSamplerState);
  if(FAILED(hr)) {
    print("D3D11: Failed to create sampler state - 0x", hex(hr), "\n");
    return false;
  } 

  return true;
}

auto D3D11Device::updateTextureAndShaderResource(u32 width, u32 height) -> bool {
  D3D11_TEXTURE2D_DESC td = {};
  td.Width = width;
  td.Height = height;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_DYNAMIC;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  td.MiscFlags = 0;

  _buffer.assign(width * height, 0);
  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = _buffer.data();
  initData.SysMemPitch = width * 4;
 
  ComPtr<ID3D11Texture2D> tex;
  hr = _pDevice->CreateTexture2D(&td, &initData, &tex);
  if(FAILED(hr)) {
    print("D3D11: Failed to create texture - 0x", hex(hr), "\n");
    return false;
  }

  hr = _pDeviceContext->Map(tex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &_mapped);
  if(FAILED(hr)) {
    print("D3D11: Failed to map texture - 0x", hex(hr), "\n");
    return false;
  }

  uint8_t* dest = reinterpret_cast<uint8_t*>(_mapped.pData);
  uint8_t* src = reinterpret_cast<uint8_t*>(_buffer.data());
  for(int y = 0; y < height; ++y) {
    memcpy(dest + y * _mapped.RowPitch, src + y * width * 4, width * 4);
  }
  _pDeviceContext->Unmap(tex.Get(), 0);

  D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
  srvd.Format = td.Format;
  srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvd.Texture2D.MostDetailedMip = 0;
  srvd.Texture2D.MipLevels = 1;

  hr = _pDevice->CreateShaderResourceView(tex.Get(), &srvd, &_pShaderResourceView);
  if(FAILED(hr)) {
    print("D3D11: Failed to create shader resource view - 0x", hex(hr), "\n");
    return false;
  } 
  
  return true;
}

auto D3D11Device::clearRenderTarget(bool present) -> void {
  float colorRGBABlack[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  if(_pDeviceContext) _pDeviceContext->ClearRenderTargetView(_pRenderTargetView.Get(), colorRGBABlack);
  if(_pSwapChain1 && present) {
    DXGI_SWAP_CHAIN_DESC1 sd1 = {};
    _pSwapChain1->GetDesc1(&sd1);
    DXGI_PRESENT_PARAMETERS pp = { 0 };
    u32 flags = 0;
    if(!_vsyncEnabled) flags = sd1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING ? DXGI_PRESENT_ALLOW_TEARING : 0;
    _pSwapChain1->Present1(_vsyncEnabled ? 1 : 0, flags, &pp);
  }
}

auto D3D11Device::render(u32 width, u32 height,  u32 windowWidth, u32 windowHeight) -> void {
  u32 offsetX = (windowWidth > width) ? ((windowWidth - width) / 2) : 0;
  u32 offsetY = (windowHeight > height) ? ((windowHeight - height) / 2) : 0;
  u32 bufferWidth = width + offsetX, bufferHeight = height + offsetY;

  DXGI_SWAP_CHAIN_DESC1 sd1 = {};
  if(!_pSwapChain1) {
    clearRenderTarget(true);
    return;
  }
  _pSwapChain1->GetDesc1(&sd1);
  if(sd1.Width != bufferWidth || sd1.Height != bufferHeight) {
    clearRenderTarget(false);
    resetRenderTargetView();
    hr = _pSwapChain1->ResizeBuffers(sd1.BufferCount, bufferWidth, bufferHeight, sd1.Format, sd1.Flags);
    if(FAILED(hr)) { print("D3D11: Failed to resize buffers - 0x", hex(hr), "\n"); return; }
    createRenderTarget();
  }
  _pDeviceContext->OMSetRenderTargets(1, _pRenderTargetView.GetAddressOf(), nullptr);

  // Define viewport
  D3D11_VIEWPORT vp = {};
  vp.TopLeftX = static_cast<FLOAT>(offsetX);
  vp.TopLeftY = static_cast<FLOAT>(offsetY);
  vp.Width = static_cast<FLOAT>(width);
  vp.Height = static_cast<FLOAT>(height);
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  _pDeviceContext->RSSetViewports(1, &vp);

  // IA
  u32 stride = sizeof(Vertex), offset = 0;
  _pDeviceContext->IASetVertexBuffers(0, 1, _pVertexBuffer.GetAddressOf(), &stride, &offset);
  _pDeviceContext->IASetIndexBuffer(_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
  _pDeviceContext->IASetInputLayout(_pInputLayout.Get());
  _pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Vertex/Pixel Shader
  _pDeviceContext->VSSetShader(_pVertexShader.Get(), nullptr, 0);
  _pDeviceContext->PSSetShader(_pPixelShader.Get(), nullptr, 0);

  // Shader Resource View / Sampler
  _pDeviceContext->PSSetShaderResources(0, 1, _pShaderResourceView.GetAddressOf());
  _pDeviceContext->PSSetSamplers(0, 1, _pSamplerState.GetAddressOf());

  // Draw
  _pDeviceContext->DrawIndexed(6, 0, 0);

  // Present
  DXGI_PRESENT_PARAMETERS pp = { 0 };
  u32 flags = 0;
  if(!_vsyncEnabled) flags = sd1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING ? DXGI_PRESENT_ALLOW_TEARING : 0;
  _pSwapChain1->Present1(_vsyncEnabled ? 1 : 0, flags, &pp);
}

auto D3D11Device::setShader(const string& pathname) -> void {
  if(_chain != NULL) {
    _libra.d3d11_filter_chain_free(&_chain);
  }

  if(_preset != NULL) {
    _libra.preset_free(&_preset);
  }

  if(file::exists(pathname)) {
    if(_libra.preset_create(pathname.data(), &_preset) != NULL) {
      print(string{"D3D11Device: Failed to load shader: ", pathname, "\n"});
      setShader("");
      return;
    }
    /*
    if(auto error = _libra.d3d11_filter_chain_create(&_preset, resolveSymbol, NULL, &_chain)) {
      print(string{"D3D11Device: Failed to create filter chain for: ", pathname, "\n"});
      _libra.error_print(error);
      setShader("");
      return;
    }*/
  }
}
