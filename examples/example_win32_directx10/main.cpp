// Dear ImGui: standalone example application for Windows API + DirectX 10

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "../../imgui_zoomable_image.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx10.h"
#include <d3d10_1.h>
#include <d3d10.h>
#include <tchar.h>

#include <stdio.h>
#include <cinttypes>
#include <stdexcept>

// Data
static ID3D10Device*            g_pd3dDevice = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D10RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
  // Make process DPI aware and obtain main monitor scale
  ImGui_ImplWin32_EnableDpiAwareness();
  float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
    ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

  // Create application window
  WNDCLASSEXW wc = {
    sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
    nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
  ::RegisterClassExW(&wc);
  HWND hwnd = ::CreateWindowW(wc.lpszClassName,
    L"Dear ImGui Zoomable Image DirectX10 Example", WS_OVERLAPPEDWINDOW,
    100, 100, (int)(1280 * main_scale), (int)(800 * main_scale),
    nullptr, nullptr, wc.hInstance, nullptr);

  // Initialize Direct3D
  if (!CreateDeviceD3D(hwnd))
  {
    CleanupDeviceD3D();
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 1;
  }

  // Show the window
  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

  // Setup Platform/Renderer backends
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX10_Init(g_pd3dDevice);

  // Create a DirectX 10 texture
  ID3D10Texture2D* texture = nullptr;
  const UINT width = 320, height = 240;
  D3D10_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA format
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D10_USAGE_DYNAMIC;
  desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
  if (!SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, nullptr, &texture)))
  {
    throw std::runtime_error("Failed to create test texture");
  }
  D3D10_MAPPED_TEXTURE2D mappedTex;
  if (!SUCCEEDED(texture->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &mappedTex)))
  {
    throw std::runtime_error("Failed to map test texture");
  }
  { // Fill the texture with a checkerboard pattern
    const size_t checkerSize = 20;
    for (size_t y = 0; y < height; ++y)
    {
      uint32_t* row = reinterpret_cast<uint32_t*>(
        static_cast<uint8_t*>(mappedTex.pData) + y * mappedTex.RowPitch);
      for (size_t x = 0; x < width; ++x)
      {
        bool isWhite = ((x / checkerSize) % 2) == ((y / checkerSize) % 2);
        uint8_t v = isWhite ? 200 : 50;
        row[x] = (0xFF << 24) | (v << 16) | (v << 8) | v; // RGBA
      }
    }
    texture->Unmap(0);
  }
  ID3D10ShaderResourceView* textureView = nullptr;
  g_pd3dDevice->CreateShaderResourceView(texture, nullptr, &textureView);

  // Our state
  ImGuiImage::State zoomState;
  zoomState.textureSize = ImVec2(width, height);
  ImVec4 clearColor{ 0.45f, 0.55f, 0.60f, 1.00f};
  ImVec2 displaySize{ 0, 0 };

  // Main loop
  bool done = false;
  while (!done)
  {
    // Poll and handle messages (inputs, window resize, etc.)
    // See the WndProc() function below for our to dispatch events to the
    // Win32 backend.
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
        done = true;
    }
    if (done)
      break;

    // Handle window being minimized or screen locked
    if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST)
      == DXGI_STATUS_OCCLUDED)
    {
      ::Sleep(10);
      continue;
    }
    g_SwapChainOccluded = false;

    // Handle window resize (we don't resize directly in the WM_SIZE handler)
    if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
    {
      CleanupRenderTarget();
      g_pSwapChain->ResizeBuffers(
        0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
      g_ResizeWidth = g_ResizeHeight = 0;
      CreateRenderTarget();
    }

    // Start the Dear ImGui frame
    ImGui_ImplDX10_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImVec2 frameSize = ImGui::GetMainViewport()->Size;
    ImVec2 imageWindowPos = ImVec2(frameSize.x * 0.1f, frameSize.y * 0.1f);
    ImVec2 imageWindowSize = ImVec2(frameSize.x * 0.5f, frameSize.y * 0.5f);
    ImVec2 controlsWindowPos = ImVec2(frameSize.x * 0.7f, frameSize.y * 0.1f);

    // Create a window to display the test image
    ImGui::SetNextWindowPos(imageWindowPos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(imageWindowSize, ImGuiCond_Once);
    {
      ImGui::Begin("Image Window");
      displaySize = ImGui::GetContentRegionAvail();
      ImGuiImage::Zoomable(textureView, displaySize, &zoomState);
      ImGui::End();
    }

    // Create a window displaying information about the test image
    ImGui::SetNextWindowPos(controlsWindowPos, ImGuiCond_Once);
    {
      ImGui::Begin("Controls Window");
      ImGui::Checkbox("Enable Zoom/Pan", &zoomState.zoomPanEnabled);
      ImGui::Checkbox("Maintain Aspect Ratio", &zoomState.maintainAspectRatio);
      if (ImGui::Button("Reset Zoom/Pan"))
      {
        zoomState.zoomLevel = 1.0f;
        zoomState.panOffset = ImVec2(0.0f, 0.0f);
      }
      ImGui::Separator();
      ImGui::Text("Texture Size: %zu x %zu", width, height);
      ImGui::Text("Display Size: %.0f x %.0f", displaySize.x, displaySize.y);
      ImGui::Text("Zoom Level: %.2f%%", zoomState.zoomLevel * 100.0f);
      ImGui::Text("Pan Offset: (%.2f, %.2f)", zoomState.panOffset.x * width,
                                              zoomState.panOffset.y * height);
      ImGui::Text("Mouse Pos: (%.2f, %.2f)", zoomState.mousePosition.x,
                                             zoomState.mousePosition.y);
      ImGui::Separator();
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / io.Framerate, io.Framerate);
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    const float colorWithAlpha[4] = {
      clearColor.x * clearColor.w, clearColor.y * clearColor.w,
      clearColor.z * clearColor.w, clearColor.w };
    g_pd3dDevice->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDevice->ClearRenderTargetView(g_mainRenderTargetView, colorWithAlpha);
    ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

    // Present
    HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
    //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
    g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
  }

  // Cleanup
  if (textureView)
    textureView->Release();
  if (texture)
    texture->Release();
  ImGui_ImplDX10_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  ::DestroyWindow(hwnd);
  ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

  return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
  // Setup swap chain
  // This is a basic setup. Optimally could use handle fullscreen mode differently.
  // See #8979 for suggestions.
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT createDeviceFlags = 0;
  //createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
  HRESULT res = D3D10CreateDeviceAndSwapChain(
    nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
    D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice);
  if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver
                                     // if hardware is not available.
    res = D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_WARP, nullptr,
      createDeviceFlags, D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice);
  if (res != S_OK)
    return false;

  CreateRenderTarget();
  return true;
}

void CleanupDeviceD3D()
{
  CleanupRenderTarget();
  if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
  if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
  ID3D10Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
  pBackBuffer->Release();
}

void CleanupRenderTarget()
{
  if (g_mainRenderTargetView)
  {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
  HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if
// dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your
//   main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to
//   your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from
// your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg)
  {
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED)
      return 0;
    g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
    g_ResizeHeight = (UINT)HIWORD(lParam);
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }
  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
