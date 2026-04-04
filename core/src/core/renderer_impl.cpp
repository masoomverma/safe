#include "core/renderer.hpp"
#include <d3d11.h>
#include <dxgi.h>

namespace safe::core {

bool Renderer::Initialize(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &m_swapChain,
        &m_device, &featureLevel, &m_deviceContext
    );

    if (hr == DXGI_ERROR_UNSUPPORTED) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, 2,
            D3D11_SDK_VERSION, &sd, &m_swapChain,
            &m_device, &featureLevel, &m_deviceContext
        );
    }

    if (hr != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void Renderer::Cleanup() {
    CleanupRenderTarget();
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_deviceContext) { m_deviceContext->Release(); m_deviceContext = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

void Renderer::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_renderTargetView);
    pBackBuffer->Release();
}

void Renderer::CleanupRenderTarget() {
    if (m_renderTargetView) { 
        m_renderTargetView->Release(); 
        m_renderTargetView = nullptr; 
    }
}

void Renderer::ResizeBuffers(UINT width, UINT height) {
    if (m_device != nullptr) {
        CleanupRenderTarget();
        m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
    }
}

void Renderer::BeginFrame(const float clearColor[4]) {
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, clearColor);
}

void Renderer::EndFrame() {
    m_swapChain->Present(1, 0);
}

} // namespace safe::core