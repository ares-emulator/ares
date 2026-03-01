
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include <thirdparty/librashader/include/librashader/librashader_ld.h>

using Microsoft::WRL::ComPtr; 

struct D3D11Device {

public:

  D3D11Device() {};
  ~D3D11Device() {};

  auto initialize(HWND context, bool blocking) -> bool;
  auto shutdown(void) -> void;
  auto createRenderTarget(void) -> bool;
  auto updateTextureAndShaderResource(u32 width, u32 height) -> bool;
  auto resetRenderTargetView(void) -> void { if(_pRenderTargetView) _pRenderTargetView.Reset(); }
  auto clearRenderTarget(bool present) -> void;
  auto clearBackBuffer(void) -> void { _buffer.clear(); }
  auto render(u32 width, u32 height, u32 windowWidth, u32 windowHeight) -> void;
  auto setShader(const string& pathname) -> void;
  auto getMappedResource(void) -> D3D11_MAPPED_SUBRESOURCE& { return _mapped; }

private:

  auto createDeviceAndSwapChain(HWND context, bool blocking) -> bool;
  auto compileShaders(void) -> bool;
  auto createGeometry(void) -> bool;
  auto createSamplerState(void) -> bool;
  
  HRESULT hr = S_OK;
  
  ComPtr<ID3D11Device>               _pDevice;
  ComPtr<ID3D11DeviceContext>        _pDeviceContext;
  ComPtr<IDXGIFactory2>              _dxgiFactory2;
  ComPtr<IDXGISwapChain1>            _pSwapChain1;
  ComPtr<ID3D11RenderTargetView>     _pRenderTargetView;
  ComPtr<ID3D11VertexShader>         _pVertexShader;
  ComPtr<ID3D11PixelShader>          _pPixelShader;
  ComPtr<ID3D11InputLayout>          _pInputLayout;
  ComPtr<ID3D11Buffer>               _pVertexBuffer;
  ComPtr<ID3D11Buffer>               _pIndexBuffer;
  ComPtr<ID3D11ShaderResourceView>   _pShaderResourceView;
  ComPtr<ID3D11SamplerState>         _pSamplerState;

  std::vector<uint32_t> _buffer;
  D3D11_MAPPED_SUBRESOURCE _mapped;
  bool _vsyncEnabled = false;
  bool _tearingSupport = false;
  struct Vertex { float x, y, z; float u, v; };

  libra_instance_t _libra;
  libra_shader_preset_t _preset = nullptr;
  libra_d3d11_filter_chain_t _chain = nullptr;
};
