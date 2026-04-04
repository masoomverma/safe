#pragma once

#include <d3d11.h>
#include <windows.h>

namespace safe::core {

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool Initialize(HWND hwnd);
    void Cleanup();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void ResizeBuffers(UINT width, UINT height);
    void BeginFrame(const float clearColor[4]);
    void EndFrame();
    
    ID3D11Device* GetDevice() const { return m_device; }
    ID3D11DeviceContext* GetDeviceContext() const { return m_deviceContext; }
    ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView; }

private:
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_deviceContext = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
};

} // namespace safe::core