auto createRenderTargets() -> bool {
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
  heapDesc.NumDescriptors = FrameCount + 1;  // +1 for the shader target when a filter chain is active
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

  HRESULT hr =
      _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_rtvHeap));
  if(FAILED(hr))
    return false;

  _rtvDescriptorSize =
      _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  auto handle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
  for(u32 index = 0; index < FrameCount; index++) {
    hr = _swapChain->GetBuffer(index, IID_PPV_ARGS(&_renderTargets[index]));
    if(FAILED(hr))
      return false;
    _device->CreateRenderTargetView(_renderTargets[index], nullptr, handle);
    handle.ptr += _rtvDescriptorSize;
  }

  _shaderTargetRtvValid = false;

  return true;
}

auto createCommandObjects() -> bool {
  HRESULT hr;
  for(u32 index = 0; index < FrameCount; index++) {
    hr = _device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&_commandAllocators[index]));
    if(FAILED(hr))
      return false;
  }

  hr = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  _commandAllocators[_frameIndex], nullptr,
                                  IID_PPV_ARGS(&_commandList));
  if(FAILED(hr))
    return false;
  // command lists start in the recording state, close immediately so beginFrame can reset it
  hr = _commandList->Close();
  if(FAILED(hr))
    return false;

  hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
  if(FAILED(hr))
    return false;

  _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if(!_fenceEvent)
    return false;

  return true;
}

auto createDescriptorHeaps() -> bool {
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
  heapDesc.NumDescriptors = 2;
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  HRESULT hr =
      _device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_srvHeap));
  if(FAILED(hr))
    return false;

  _srvDescriptorSize = _device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  return true;
}

auto createPipeline() -> bool {
  struct CompiledShader {
    IUnknown *blob = nullptr;
    const void *bytecode = nullptr;
    SIZE_T length = 0;
  };

  auto releaseCompiledShader = [&](CompiledShader &shader) -> void {
    d3d12device_release(shader.blob);
    shader.bytecode = nullptr;
    shader.length = 0;
  };

  auto compileWithFxc = [&](const char *source, const char *profile,
                            CompiledShader &output) -> bool {
    ID3DBlob *shader = nullptr;
    ID3DBlob *errors = nullptr;
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    auto sourceLength = string{source}.size();
    auto result =
        D3DCompile(source, sourceLength, nullptr, nullptr, nullptr, "main",
                   profile, compileFlags, 0, &shader, &errors);
    if(FAILED(result)) {
      if(errors) {
        print(string{"Direct3D 12: FXC compile failed for ", profile, ":\n"});
        print((const char *)errors->GetBufferPointer());
        print("\n");
        d3d12device_release(errors);
      }
      return false;
    }

    if(errors)
      d3d12device_release(errors);

    output.blob = shader;
    output.bytecode = shader->GetBufferPointer();
    output.length = shader->GetBufferSize();
    return true;
  };

  static const char *vertexShaderSource = R"(
struct VSInput {
  float4 position : POSITION;
  float2 uv : TEXCOORD;
};

struct VSOutput {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD;
};

VSOutput main(VSInput input) {
  VSOutput output;
  output.position = input.position;
  output.uv = input.uv;
  return output;
}
)";

  static const char *pixelShaderSource = R"(
Texture2D sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET {
  return sourceTexture.Sample(sourceSampler, uv);
}
)";

  CompiledShader vertexShader{};
  CompiledShader pixelShader{};

  if(!compileWithFxc(vertexShaderSource, "vs_5_0", vertexShader)) {
    releaseCompiledShader(vertexShader);
    return false;
  }

  if(!compileWithFxc(pixelShaderSource, "ps_5_0", pixelShader)) {
    releaseCompiledShader(vertexShader);
    releaseCompiledShader(pixelShader);
    return false;
  }

  D3D12_STATIC_SAMPLER_DESC sampler{};
  sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  sampler.ShaderRegister = 0;
  sampler.RegisterSpace = 0;
  sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  sampler.MaxLOD = D3D12_FLOAT32_MAX;

  ID3DBlob *serializedRootSignature = nullptr;
  ID3DBlob *rootSignatureErrors = nullptr;
  HRESULT rootSignatureStatus = E_FAIL;

  if(_supportsRootSignature11) {
    D3D12_DESCRIPTOR_RANGE1 range1{};
    range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range1.NumDescriptors = 1;
    range1.BaseShaderRegister = 0;
    range1.RegisterSpace = 0;
    range1.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
    range1.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER1 parameter1{};
    parameter1.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter1.DescriptorTable.NumDescriptorRanges = 1;
    parameter1.DescriptorTable.pDescriptorRanges = &range1;
    parameter1.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc1{};
    rootSignatureDesc1.NumParameters = 1;
    rootSignatureDesc1.pParameters = &parameter1;
    rootSignatureDesc1.NumStaticSamplers = 1;
    rootSignatureDesc1.pStaticSamplers = &sampler;
    rootSignatureDesc1.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{};
    versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    versionedDesc.Desc_1_1 = rootSignatureDesc1;

    rootSignatureStatus = D3D12SerializeVersionedRootSignature(
        &versionedDesc, &serializedRootSignature, &rootSignatureErrors);
  } else {
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    range.RegisterSpace = 0;
    range.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable.NumDescriptorRanges = 1;
    parameter.DescriptorTable.pDescriptorRanges = &range;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &parameter;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &sampler;
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{};
    versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
    versionedDesc.Desc_1_0 = rootSignatureDesc;

    rootSignatureStatus = D3D12SerializeVersionedRootSignature(
        &versionedDesc, &serializedRootSignature, &rootSignatureErrors);
  }

  if(FAILED(rootSignatureStatus)) {
    if(rootSignatureErrors)
      d3d12device_release(rootSignatureErrors);
    releaseCompiledShader(vertexShader);
    releaseCompiledShader(pixelShader);
    return false;
  }

  if(rootSignatureErrors)
    d3d12device_release(rootSignatureErrors);

  HRESULT hr = _device->CreateRootSignature(
      0, serializedRootSignature->GetBufferPointer(),
      serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
  if(FAILED(hr)) {
    d3d12device_release(serializedRootSignature);
    releaseCompiledShader(vertexShader);
    releaseCompiledShader(pixelShader);
    return false;
  }

  d3d12device_release(serializedRootSignature);

  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  D3D12_BLEND_DESC blendDesc{};
  auto &targetBlend = blendDesc.RenderTarget[0];
  targetBlend.SrcBlend = D3D12_BLEND_ONE;
  targetBlend.DestBlend = D3D12_BLEND_ZERO;
  targetBlend.BlendOp = D3D12_BLEND_OP_ADD;
  targetBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
  targetBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
  targetBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  targetBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
  targetBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
  rasterizerDesc.DepthClipEnable = TRUE;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
  pipelineDesc.pRootSignature = _rootSignature;
  pipelineDesc.VS = {vertexShader.bytecode, vertexShader.length};
  pipelineDesc.PS = {pixelShader.bytecode, pixelShader.length};
  pipelineDesc.BlendState = blendDesc;
  pipelineDesc.SampleMask = UINT_MAX;
  pipelineDesc.RasterizerState = rasterizerDesc;
  pipelineDesc.InputLayout = {
      inputLayout,
      (UINT)(sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC))};
  pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipelineDesc.NumRenderTargets = 1;
  pipelineDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
  pipelineDesc.SampleDesc.Count = 1;

  hr = _device->CreateGraphicsPipelineState(&pipelineDesc,
                                            IID_PPV_ARGS(&_pipelineState));

  releaseCompiledShader(vertexShader);
  releaseCompiledShader(pixelShader);

  return SUCCEEDED(hr);
}

auto createVertexBuffer() -> bool {
  Vertex vertices[] = {
      {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
      {{1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      {{1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
  };

  auto bufferSize = sizeof(vertices);

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = bufferSize;
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  HRESULT hr = _device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer));
  if(FAILED(hr))
    return false;

  void *mappedData = nullptr;
  D3D12_RANGE readRange{0, 0};
  hr = _vertexBuffer->Map(0, &readRange, &mappedData);
  if(FAILED(hr))
    return false;

  memory::copy(mappedData, vertices, bufferSize);
  _vertexBuffer->Unmap(0, nullptr);

  _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
  _vertexBufferView.StrideInBytes = sizeof(Vertex);
  _vertexBufferView.SizeInBytes = (UINT)bufferSize;

  return true;
}

auto resizeSource(u32 width, u32 height) -> bool {
  _sourceWidth = width;
  _sourceHeight = height;

  resetSrvDescriptorCache();

  d3d12device_release(_sourceTexture);
  d3d12device_release(_sourceUpload);

  D3D12_RESOURCE_DESC textureDesc{};
  textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  textureDesc.Alignment = 0;
  textureDesc.Width = width;
  textureDesc.Height = height;
  textureDesc.DepthOrArraySize = 1;
  textureDesc.MipLevels = 1;
  textureDesc.Format = _sourceFormat;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

  D3D12_HEAP_PROPERTIES textureHeap{};
  textureHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

  HRESULT hr = _device->CreateCommittedResource(
      &textureHeap, D3D12_HEAP_FLAG_NONE, &textureDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_sourceTexture));
  if(FAILED(hr))
    return false;

  _sourceState = D3D12_RESOURCE_STATE_COPY_DEST;

  UINT64 uploadSize = 0;
  _device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &_sourceFootprint,
                                 nullptr, nullptr, &uploadSize);

  D3D12_RESOURCE_DESC uploadDesc{};
  uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  uploadDesc.Width = uploadSize;
  uploadDesc.Height = 1;
  uploadDesc.DepthOrArraySize = 1;
  uploadDesc.MipLevels = 1;
  uploadDesc.SampleDesc.Count = 1;
  uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  D3D12_HEAP_PROPERTIES uploadHeap{};
  uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

  hr = _device->CreateCommittedResource(
      &uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_sourceUpload));
  if(FAILED(hr))
    return false;

  return true;
}

auto uploadSourceTexture(const u32 *sourceData, u32 sourceWidth,
                         u32 sourceHeight) -> bool {
  if(!_sourceTexture || !_sourceUpload || !sourceData)
    return false;

  transitionResource(_sourceTexture, _sourceState,
                     D3D12_RESOURCE_STATE_COPY_DEST);
  _sourceState = D3D12_RESOURCE_STATE_COPY_DEST;

  void *mapped = nullptr;
  D3D12_RANGE readRange{0, 0};
  HRESULT hr = _sourceUpload->Map(0, &readRange, &mapped);
  if(FAILED(hr))
    return false;

  auto destination = (u8 *)mapped + _sourceFootprint.Offset;
  auto source = (const u8 *)sourceData;
  u32 sourcePitch = sourceWidth * sizeof(u32);

  // GPU row pitch may be wider than source due to alignment, copy row by row
  for(u32 y : range(sourceHeight)) {
    memory::copy(destination + y * _sourceFootprint.Footprint.RowPitch,
                 source + y * sourcePitch, sourcePitch);
  }

  _sourceUpload->Unmap(0, nullptr);

  D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
  destinationLocation.pResource = _sourceTexture;
  destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  destinationLocation.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
  sourceLocation.pResource = _sourceUpload;
  sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  sourceLocation.PlacedFootprint = _sourceFootprint;

  _commandList->CopyTextureRegion(&destinationLocation, 0, 0, 0,
                                  &sourceLocation, nullptr);

  transitionResource(_sourceTexture, _sourceState,
                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  _sourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

  return true;
}

auto ensureShaderTarget(u32 width, u32 height) -> bool {
  if(!_filterChain)
    return true;

  if(_shaderTarget && _shaderWidth == width && _shaderHeight == height &&
      _shaderTargetRtvValid)
    return true;

  auto recreateResource =
      !_shaderTarget || _shaderWidth != width || _shaderHeight != height;
  if(recreateResource) {
    resetSrvDescriptorCache();
    d3d12device_release(_shaderTarget);
    _shaderWidth = width;
    _shaderHeight = height;
  }

  D3D12_RESOURCE_DESC textureDesc{};
  textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  textureDesc.Alignment = 0;
  textureDesc.Width = width;
  textureDesc.Height = height;
  textureDesc.DepthOrArraySize = 1;
  textureDesc.MipLevels = 1;
  textureDesc.Format = _shaderTargetFormat;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_CLEAR_VALUE clearValue{};
  clearValue.Format = _shaderTargetFormat;
  clearValue.Color[0] = 0.0f;
  clearValue.Color[1] = 0.0f;
  clearValue.Color[2] = 0.0f;
  clearValue.Color[3] = 1.0f;

  if(recreateResource) {
    HRESULT hr = _device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue,
        IID_PPV_ARGS(&_shaderTarget));
    if(FAILED(hr))
      return false;

    _shaderTargetState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  }

  auto shaderTargetRtvHandle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
  shaderTargetRtvHandle.ptr += FrameCount * _rtvDescriptorSize;
  _device->CreateRenderTargetView(_shaderTarget, nullptr,
                                  shaderTargetRtvHandle);
  _shaderTargetRtvValid = true;

  return true;
}

auto updateSourceFormat(const string &format) -> bool {
  if(format == "ARGB24") {
    _sourceFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    _shaderTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    resetSrvDescriptorCache();
    return true;
  }
  if(format == "ARGB30") {
    _sourceFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    _shaderTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    resetSrvDescriptorCache();
    return true;
  }
  return false;
}
