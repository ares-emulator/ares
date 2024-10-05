// Adapted from https://gist.github.com/d7samurai/abab8a580d0298cb2f34a44eec41d39d
// 
// This example shows how librashader can be integrated into a more advanced renderer.
// 
// Whereas the first example freed and recreated a texture every frame, this uses a persistent texture for better performance.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

///////////////////////////////////////////////////////////////////////////////////////////////////

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <windows.h>
#define LIBRA_RUNTIME_D3D11
#include "../../../../include/librashader_ld.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

#define TITLE "Minimal D3D11 pt3 by d7samurai"

///////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {
    WNDCLASSA wndClass = {0, DefWindowProcA, 0, 0, 0, 0, 0, 0, 0, TITLE};

    RegisterClassA(&wndClass);

    HWND window =
        CreateWindowExA(0, TITLE, TITLE, WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0,
                        0, 0, 0, nullptr, nullptr, nullptr, nullptr);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};

    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, featureLevels,
                      ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice,
                      nullptr, &baseDeviceContext);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Device1* device;

    baseDevice->QueryInterface(__uuidof(ID3D11Device1),
                               reinterpret_cast<void**>(&device));

    ID3D11DeviceContext1* deviceContext;

    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1),
                                      reinterpret_cast<void**>(&deviceContext));

    ID3D11DeviceContext* deferredContext;
    baseDevice->CreateDeferredContext(0, &deferredContext);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    auto libra = librashader_load_instance();
    libra_shader_preset_t preset;
    auto error = libra.preset_create(
        "../../../shaders_slang/crt/crt-royale.slangp",
        &preset);

    libra_d3d11_filter_chain_t filter_chain;
    filter_chain_d3d11_opt_t opt = {
        .force_no_mipmaps = false,
    };

    libra.d3d11_filter_chain_create(&preset, device, &opt, &filter_chain);
    ///////////////////////////////////////////////////////////////////////////////////////////////
    IDXGIDevice1* dxgiDevice;

    device->QueryInterface(__uuidof(IDXGIDevice1),
                           reinterpret_cast<void**>(&dxgiDevice));

    IDXGIAdapter* dxgiAdapter;

    dxgiDevice->GetAdapter(&dxgiAdapter);

    IDXGIFactory2* dxgiFactory;

    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2),
                           reinterpret_cast<void**>(&dxgiFactory));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = 0;     // use window width
    swapChainDesc.Height = 0;    // use window height
    swapChainDesc.Format =
        DXGI_FORMAT_B8G8R8A8_UNORM;    // can't specify _SRGB here when using
                                       // DXGI_SWAP_EFFECT_FLIP_* ...
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    IDXGISwapChain1* swapChain;

    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr,
                                        nullptr, &swapChain);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* framebufferTexture;
    ID3D11Texture2D* framebufferCopy;

    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                         reinterpret_cast<void**>(&framebufferTexture));

    D3D11_TEXTURE2D_DESC framebufferTextureDesc;
    framebufferTexture->GetDesc(&framebufferTextureDesc);
    framebufferTextureDesc.BindFlags =
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    framebufferTextureDesc.CPUAccessFlags =
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_READ;

    device->CreateTexture2D(&framebufferTextureDesc, nullptr,
                                      &framebufferCopy);

    D3D11_RENDER_TARGET_VIEW_DESC framebufferDesc = {};
    framebufferDesc.Format =
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;    // ... so do this to get _SRGB
                                            // swapchain
    framebufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    ID3D11RenderTargetView* framebufferRTV;

    device->CreateRenderTargetView(framebufferTexture, &framebufferDesc,
                                   &framebufferRTV);

    
    
    ID3D11ShaderResourceView* framebufferCopySRV;
    device->CreateShaderResourceView(framebufferCopy, 0, &framebufferCopySRV);
    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC framebufferDepthDesc;

    framebufferTexture->GetDesc(
        &framebufferDepthDesc);    // copy from framebuffer properties

    framebufferDepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    framebufferDepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* framebufferDepthTexture;

    device->CreateTexture2D(&framebufferDepthDesc, nullptr,
                            &framebufferDepthTexture);

    ID3D11DepthStencilView* framebufferDSV;

    device->CreateDepthStencilView(framebufferDepthTexture, nullptr,
                                   &framebufferDSV);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC shadowmapDepthDesc = {};
    shadowmapDepthDesc.Width = 2048;
    shadowmapDepthDesc.Height = 2048;
    shadowmapDepthDesc.MipLevels = 1;
    shadowmapDepthDesc.ArraySize = 1;
    shadowmapDepthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    shadowmapDepthDesc.SampleDesc.Count = 1;
    shadowmapDepthDesc.Usage = D3D11_USAGE_DEFAULT;
    shadowmapDepthDesc.BindFlags =
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* shadowmapDepthTexture;

    device->CreateTexture2D(&shadowmapDepthDesc, nullptr,
                            &shadowmapDepthTexture);

    D3D11_DEPTH_STENCIL_VIEW_DESC shadowmapDSVdesc = {};
    shadowmapDSVdesc.Format = DXGI_FORMAT_D32_FLOAT;
    shadowmapDSVdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    ID3D11DepthStencilView* shadowmapDSV;

    device->CreateDepthStencilView(shadowmapDepthTexture, &shadowmapDSVdesc,
                                   &shadowmapDSV);

    D3D11_SHADER_RESOURCE_VIEW_DESC shadowmapSRVdesc = {};
    shadowmapSRVdesc.Format = DXGI_FORMAT_R32_FLOAT;
    shadowmapSRVdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shadowmapSRVdesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* shadowmapSRV;

    device->CreateShaderResourceView(shadowmapDepthTexture, &shadowmapSRVdesc,
                                     &shadowmapSRV);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    struct float4 {
        float x, y, z, w;
    };

    struct Constants {
        float4 CameraProjection[4];
        float4 LightProjection[4];
        float4 LightRotation;
        float4 ModelRotation;
        float4 ModelTranslation;
        float4 ShadowmapSize;
    };

    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth =
        sizeof(Constants) + 0xf &
        0xfffffff0;    // ensure constant buffer size is multiple of 16 bytes
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* constantBuffer;

    device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    float vertexData[] = {
        -1,    1,      -1,     0,      0,     1,      1,     -1,    9.5f,
        0,     -0.58f, 0.58f,  -1,     2,     2,      0.58f, 0.58f, -1,
        7.5f,  2,      -0.58f, 0.58f,  -1,    0,      0,     0.58f, 0.58f,
        -1,    0,      0,      -0.58f, 0.58f, -0.58f, 0,     0,     0.58f,
        0.58f, -0.58f, 0,      0};    // pos.x, pos.y, pos.z, tex.u, tex.v, ...

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(vertexData);
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags =
        D3D11_BIND_SHADER_RESOURCE;    // using regular shader resource as
                                       // vertex buffer for manual vertex fetch
    vertexBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    vertexBufferDesc.StructureByteStride =
        5 *
        sizeof(
            float);    // 5 floats per vertex (float3 position, float2 texcoord)

    D3D11_SUBRESOURCE_DATA vertexBufferData = {vertexData};

    ID3D11Buffer* vertexBuffer;

    device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);

    D3D11_SHADER_RESOURCE_VIEW_DESC vertexBufferSRVdesc = {};
    vertexBufferSRVdesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexBufferSRVdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    vertexBufferSRVdesc.Buffer.NumElements =
        vertexBufferDesc.ByteWidth / vertexBufferDesc.StructureByteStride;

    ID3D11ShaderResourceView* vertexBufferSRV;

    device->CreateShaderResourceView(vertexBuffer, &vertexBufferSRVdesc,
                                     &vertexBufferSRV);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

    ID3D11DepthStencilState* depthStencilState;

    device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_RASTERIZER_DESC1 rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    ID3D11RasterizerState1* cullBackRS;

    device->CreateRasterizerState1(&rasterizerDesc, &cullBackRS);

    rasterizerDesc.CullMode = D3D11_CULL_FRONT;

    ID3D11RasterizerState1* cullFrontRS;

    device->CreateRasterizerState1(&rasterizerDesc, &cullFrontRS);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* framebufferVSBlob;

    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "framebuffer_vs",
                       "vs_5_0", 0, 0, &framebufferVSBlob, nullptr);

    ID3D11VertexShader* framebufferVS;

    device->CreateVertexShader(framebufferVSBlob->GetBufferPointer(),
                               framebufferVSBlob->GetBufferSize(), nullptr,
                               &framebufferVS);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* framebufferPSBlob;

    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "framebuffer_ps",
                       "ps_5_0", 0, 0, &framebufferPSBlob, nullptr);

    ID3D11PixelShader* framebufferPS;

    device->CreatePixelShader(framebufferPSBlob->GetBufferPointer(),
                              framebufferPSBlob->GetBufferSize(), nullptr,
                              &framebufferPS);

    /////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* shadowmapVSBlob;

    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "shadowmap_vs",
                       "vs_5_0", 0, 0, &shadowmapVSBlob, nullptr);

    ID3D11VertexShader* shadowmapVS;

    device->CreateVertexShader(shadowmapVSBlob->GetBufferPointer(),
                               shadowmapVSBlob->GetBufferSize(), nullptr,
                               &shadowmapVS);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    FLOAT framebufferClear[4] = {0.025f, 0.025f, 0.025f, 1};

    D3D11_VIEWPORT framebufferVP = {
        0,
        0,
        static_cast<float>(framebufferDepthDesc.Width),
        static_cast<float>(framebufferDepthDesc.Height),
        0,
        1};
    D3D11_VIEWPORT shadowmapVP = {0,
                                  0,
                                  static_cast<float>(shadowmapDepthDesc.Width),
                                  static_cast<float>(shadowmapDepthDesc.Height),
                                  0,
                                  1};

    ID3D11ShaderResourceView* nullSRV =
        nullptr;    // null srv used for unbinding resources

    ///////////////////////////////////////////////////////////////////////////////////////////////

    Constants constants = {2.0f / (framebufferVP.Width / framebufferVP.Height),
                           0,
                           0,
                           0,
                           0,
                           2,
                           0,
                           0,
                           0,
                           0,
                           1.125f,
                           1,
                           0,
                           0,
                           -1.125f,
                           0,    // camera projection matrix (perspective)
                           0.5f,
                           0,
                           0,
                           0,
                           0,
                           0.5f,
                           0,
                           0,
                           0,
                           0,
                           0.125f,
                           0,
                           0,
                           0,
                           -0.125f,
                           1};    // light projection matrix (orthographic)

    constants.LightRotation = {0.8f, 0.6f, 0.0f};
    constants.ModelRotation = {0.0f, 0.0f, 0.0f};
    constants.ModelTranslation = {0.0f, 0.0f, 4.0f};

    constants.ShadowmapSize = {shadowmapVP.Width, shadowmapVP.Height};

    ///////////////////////////////////////////////////////////////////////////////////////////////

    size_t frameCount = 0;
    while (true) {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_KEYDOWN) return 0;
            DispatchMessageA(&msg);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////

        constants.ModelRotation.x += 0.001f;
        constants.ModelRotation.y += 0.005f;
        constants.ModelRotation.z += 0.003f;

        ///////////////////////////////////////////////////////////////////////////////////////////

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;

        deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0,
                           &mappedSubresource);

        *reinterpret_cast<Constants*>(mappedSubresource.pData) = constants;

        deviceContext->Unmap(constantBuffer, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->ClearDepthStencilView(shadowmapDSV, D3D11_CLEAR_DEPTH,
                                             1.0f, 0);

        deviceContext->OMSetRenderTargets(
            0, nullptr, shadowmapDSV);    // null rendertarget for depth only
        deviceContext->OMSetDepthStencilState(depthStencilState, 0);

        deviceContext->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);    // using triangle strip
                                                        // this time

        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);
        deviceContext->VSSetShaderResources(0, 1, &vertexBufferSRV);
        deviceContext->VSSetShader(shadowmapVS, nullptr, 0);

        deviceContext->RSSetViewports(1, &shadowmapVP);
        deviceContext->RSSetState(cullFrontRS);

        deviceContext->PSSetShader(nullptr, nullptr,
                                   0);    // null pixelshader for depth only

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->DrawInstanced(8, 24, 0,
                                     0);    // render shadowmap (light pov)

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->ClearRenderTargetView(framebufferRTV, framebufferClear);
        deviceContext->ClearDepthStencilView(framebufferDSV, D3D11_CLEAR_DEPTH,
                                             1.0f, 0);

        deviceContext->OMSetRenderTargets(1, &framebufferRTV, framebufferDSV);

        deviceContext->VSSetShader(framebufferVS, nullptr, 0);

        deviceContext->RSSetViewports(1, &framebufferVP);
        deviceContext->RSSetState(cullBackRS);

        deviceContext->PSSetShaderResources(1, 1, &shadowmapSRV);
        deviceContext->PSSetShader(framebufferPS, nullptr, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->DrawInstanced(8, 24, 0,
                                     0);    // render framebuffer (camera pov)

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->PSSetShaderResources(
            1, 1,
            &nullSRV);    // release shadowmap as srv to avoid srv/dsv conflict

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->CopyResource(framebufferCopy, framebufferTexture);

        frame_d3d11_opt_t frame_opt = {.clear_history = false,
                                   .frame_direction = -1};

        libra.d3d11_filter_chain_frame(&filter_chain, deferredContext,
                                       frameCount, framebufferCopySRV,
                                       framebufferRTV, NULL, 
                                        NULL, &frame_opt);

        ID3D11CommandList* commandList;
        deferredContext->FinishCommandList(false, &commandList);
        deviceContext->ExecuteCommandList(commandList, true);
        ////////////////////////////////////////////////////////////////////////
        swapChain->Present(1, 0);
        frameCount += 1;
    }
}