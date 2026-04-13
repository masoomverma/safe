/**
 * Safe - Secure Folder Management System
 * 
 * A Windows-based application for encrypting and managing files/folders
 * Built with DirectX 11 and Dear ImGui
 * 
 * Current Phase: 3 - UI Implementation with Lock/Unlock operations
 * 
 * Features:
 * - Modern UI with ImGui
 * - Multiple item selection
 * - Lock/Unlock operations (encryption placeholder)
 * - Search functionality
 * - State-driven UI
 * 
 * Architecture:
 * - main.cpp: Application entry point, DirectX/ImGui setup
 * - ui.cpp: UI logic and rendering
 * - Core modules: Filesystem, Folder management (Phase 4+)
 */

#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>

#define IDI_ICON1 101

// ImGui libs
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// safe headers
#include "ui/ui.hpp"

// Using Microsoft::WRL::ComPtr for automatic COM resource management
using Microsoft::WRL::ComPtr;

// DIRECTX 11 GLOBALS

// DirectX 11 globals (using ComPtr for automatic resource management)
static ComPtr<ID3D11Device>            g_pd3dDevice;
static ComPtr<ID3D11DeviceContext>     g_pd3dDeviceContext;
static ComPtr<IDXGISwapChain>          g_pSwapChain;
static ComPtr<ID3D11RenderTargetView>  g_mainRenderTargetView;

// FORWARD DECLARATIONS

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DIRECTX CONFIGURATION

// Constants for DirectX initialization
constexpr UINT BUFFER_COUNT = 2;
constexpr DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr UINT REFRESH_RATE_NUMERATOR = 60;
constexpr UINT REFRESH_RATE_DENOMINATOR = 1;
constexpr UINT SAMPLE_COUNT = 1;
constexpr UINT SAMPLE_QUALITY = 0;
constexpr BOOL WINDOWED = TRUE;
constexpr DXGI_SWAP_EFFECT SWAP_EFFECT = DXGI_SWAP_EFFECT_DISCARD;
constexpr UINT SWAP_CHAIN_FLAGS = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

// Globals for clear color
static auto g_clearColor = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
static bool g_SwapChainOccluded = false;

// HELPER FUNCTIONS

/**
 * Logs error message to debug output and shows message box
 */
static void LogError(const wchar_t* message, const wchar_t* title = L"Error")
{
#ifdef _DEBUG
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
#endif
    MessageBoxW(nullptr, message, title, MB_ICONERROR | MB_OK);
}

// DIRECTX 11 SETUP

/**
 * Creates DirectX 11 device and swap chain
 * Falls back to WARP if hardware device not supported
 */
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = BUFFER_COUNT;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = BACKBUFFER_FORMAT;
    sd.BufferDesc.RefreshRate.Numerator   = REFRESH_RATE_NUMERATOR;
    sd.BufferDesc.RefreshRate.Denominator = REFRESH_RATE_DENOMINATOR;
    sd.Flags                              = SWAP_CHAIN_FLAGS;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = SAMPLE_COUNT;
    sd.SampleDesc.Quality                 = SAMPLE_QUALITY;
    sd.Windowed                           = WINDOWED;
    sd.SwapEffect                         = SWAP_EFFECT;

    constexpr UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    constexpr D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, _countof(featureLevelArray),
        D3D11_SDK_VERSION, &sd, g_pSwapChain.GetAddressOf(),
        g_pd3dDevice.GetAddressOf(), &featureLevel, g_pd3dDeviceContext.GetAddressOf()
    );

    if (res == DXGI_ERROR_UNSUPPORTED)
    {
        OutputDebugStringW(L"Hardware device not supported, falling back to WARP\n");
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            createDeviceFlags, featureLevelArray, _countof(featureLevelArray),
            D3D11_SDK_VERSION, &sd, g_pSwapChain.GetAddressOf(),
            g_pd3dDevice.GetAddressOf(), &featureLevel, g_pd3dDeviceContext.GetAddressOf()
        );
    }

    if (res != S_OK)
    {
        LogError(L"Failed to create DirectX 11 device and swap chain", L"DirectX Error");
        return false;
    }

    CreateRenderTarget();
    return true;
}

/**
 * Cleanup DirectX resources
 */
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    g_pSwapChain.Reset();
    g_pd3dDeviceContext.Reset();
    g_pd3dDevice.Reset();
}

/**
 * Creates render target view from swap chain back buffer
 */
void CreateRenderTarget()
{
    if (!g_pSwapChain || !g_pd3dDevice)
    {
        OutputDebugStringW(L"CreateRenderTarget: SwapChain or Device is null\n");
        return;
    }

    // Reset existing render target view (ComPtr handles cleanup automatically)
    g_mainRenderTargetView.Reset();

    ComPtr<ID3D11Texture2D> pBackBuffer;
    HRESULT hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
    if (FAILED(hr))
    {
        OutputDebugStringW(L"Failed to get back buffer from swap chain\n");
        return;
    }

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, g_mainRenderTargetView.GetAddressOf());
    if (FAILED(hr))
    {
        OutputDebugStringW(L"Failed to create render target view\n");
        g_mainRenderTargetView.Reset();
    }
}

/**
 * Cleanup render target view
 */
void CleanupRenderTarget()
{
    g_mainRenderTargetView.Reset();
}

// WINDOW PROCEDURE

/**
 * Windows message handler
 * Handles window events and forwards ImGui events
 */
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() != nullptr)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return 0;
    }

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice && g_pSwapChain && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            HRESULT hr = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(hr))
            {
                CreateRenderTarget();
            }
            else
            {
                OutputDebugStringW(L"Failed to resize swap chain buffers\n");
                // Attempt to recreate the swap chain if resize fails
                if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
                {
                    LogError(L"DirectX device lost during resize. Please restart the application.", L"Device Error");
                }
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// APPLICATION ENTRY POINT

/**
 * WinMain - Application entry point
 * 
 * Initialization sequence:
 * 1. Register window class
 * 2. Create window
 * 3. Initialize DirectX 11
 * 4. Initialize ImGui
 * 5. Initialize Safe UI
 * 6. Main render loop
 * 7. Cleanup
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = wc.hIcon;
    wc.lpszClassName = L"SafeApp";
    
    if (!RegisterClassExW(&wc))
    {
        LogError(L"Failed to register window class", L"Registration Error");
        return 1;
    }

    // Create window
    HWND hWnd = CreateWindowExW(
        0, wc.lpszClassName, L"Safe",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 660,
        nullptr, nullptr, hInstance, nullptr
    );

    // Validate window creation
    if (!hWnd)
    {
        LogError(L"Failed to create application window", L"Window Creation Error");
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    // Application Windows Behavior Design
    LONG winStyle = GetWindowLong(hWnd, GWL_STYLE);
    winStyle &= ~WS_MAXIMIZEBOX;
    SetWindowLong(hWnd, GWL_STYLE, winStyle);
    SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    // Init Direct3D
    if (!CreateDeviceD3D(hWnd))
    {
        LogError(L"Failed to initialize DirectX 11. Please ensure your graphics drivers are up to date.", L"DirectX Initialization Error");
        CleanupDeviceD3D();
        DestroyWindow(hWnd);
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // IMGUI INITIALIZATION

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply light theme style
    ImGui::StyleColorsLight();
    ImGuiStyle& imgui_style = ImGui::GetStyle();
    imgui_style.WindowRounding = 8.0f;
    imgui_style.FrameRounding = 4.0f;
    imgui_style.ChildRounding = 8.0f;
    imgui_style.WindowBorderSize = 0.0f;
    imgui_style.ChildBorderSize = 1.0f;  // Enable child window borders
    imgui_style.FrameBorderSize = 1.0f;
    imgui_style.WindowPadding = ImVec2(12.0f, 10.0f);
    imgui_style.FramePadding = ImVec2(10.0f, 6.0f);
    imgui_style.ItemSpacing = ImVec2(10.0f, 8.0f);

    ImVec4* colors = imgui_style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.96f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);  // White for better contrast
    colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);  // Visible borders
    colors[ImGuiCol_Header] = ImVec4(0.34f, 0.54f, 0.86f, 1.0f);         // Selected item (medium blue)
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.58f, 0.76f, 0.98f, 1.0f);  // Hovered item (lighter than selected)
    colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.44f, 0.78f, 1.0f);   // Clicked/active item
    colors[ImGuiCol_Button] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.88f, 0.88f, 0.88f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.0f);

    // Initialize backends
    if (!ImGui_ImplWin32_Init(hWnd))
    {
        LogError(L"Failed to initialize ImGui Win32 backend", L"ImGui Error");
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        DestroyWindow(hWnd);
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }
    
    if (!ImGui_ImplDX11_Init(g_pd3dDevice.Get(), g_pd3dDeviceContext.Get()))
    {
        LogError(L"Failed to initialize ImGui DirectX 11 backend", L"ImGui Error");
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        DestroyWindow(hWnd);
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    // Set clear color
    g_clearColor = colors[ImGuiCol_WindowBg];

    // SAFE UI INITIALIZATION

    // Initialize Safe UI
    if (!safe::ui::UI::Initialize())
    {
        LogError(L"Failed to initialize UI system", L"UI Error");
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        DestroyWindow(hWnd);
        UnregisterClassW(wc.lpszClassName, hInstance);
        return 1;
    }

    // MAIN RENDER LOOP

    // Main loop
    MSG msg = {};
    
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
        }
        if (msg.message == WM_QUIT) break;

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // SAFE UI RENDERING - Phase 3
        safe::ui::UI::Render();


        // Render
        ImGui::Render();
        
        // Validate DirectX resources before rendering
        if (g_pd3dDeviceContext && g_mainRenderTargetView && g_pSwapChain)
        {
            const float clear[4] = { g_clearColor.x, g_clearColor.y, g_clearColor.z, g_clearColor.w };
            ID3D11RenderTargetView* rtv = g_mainRenderTargetView.Get();
            g_pd3dDeviceContext->OMSetRenderTargets(1, &rtv, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView.Get(), clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            
            // Present with handling for occlusion
            HRESULT hr = g_pSwapChain->Present(1, 0);
            g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
            
            // Log errors (except occlusion which is normal)
            if (hr != S_OK && hr != DXGI_STATUS_OCCLUDED)
            {
                OutputDebugStringW(L"Present failed\n");
            }
        }
    }

    // CLEANUP

    // Cleanup
    safe::ui::UI::Cleanup();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();

    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, hInstance);

    return 0;
}
