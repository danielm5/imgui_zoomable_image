// Dear ImGui: standalone example application for Windows API + DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "../../imgui_zoomable_image.h"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>

#include <stdio.h>
#include <cinttypes>

// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
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
    L"Dear ImGui Zoomable Image DirectX9 Example", WS_OVERLAPPEDWINDOW,
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
  ImGui_ImplDX9_Init(g_pd3dDevice);

  // Create a DirectX 9 texture
  LPDIRECT3DTEXTURE9 texture = nullptr;
  const UINT width = 320, height = 240;
  if (SUCCEEDED(g_pd3dDevice->CreateTexture(width, height, 1, 0,
    D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr)))
  {
    D3DLOCKED_RECT lockedRect;
    if (SUCCEEDED(texture->LockRect(0, &lockedRect, nullptr, 0)))
    {
      const size_t checkerSize = 20;
      uint8_t* pBits = static_cast<uint8_t*>(lockedRect.pBits);
      for (size_t y = 0; y < height; ++y)
      {
        uint32_t* row = reinterpret_cast<uint32_t*>(pBits + y * lockedRect.Pitch);
        for (size_t x = 0; x < width; ++x)
        {
          bool isWhite = ((x / checkerSize) % 2) == ((y / checkerSize) % 2);
          uint8_t v = isWhite ? 200 : 50;
          row[x] = 0xFF000000 | (v << 16) | (v << 8) | v; // ARGB
        }
      }
    texture->UnlockRect(0);
    }
  }

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
    // See the WndProc() function below for our to dispatch events to the Win32 backend.
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

    // Handle lost D3D9 device
    if (g_DeviceLost)
    {
      HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
      if (hr == D3DERR_DEVICELOST)
      {
        ::Sleep(10);
        continue;
      }
      if (hr == D3DERR_DEVICENOTRESET)
        ResetDevice();
      g_DeviceLost = false;
    }

    // Handle window resize (we don't resize directly in the WM_SIZE handler)
    if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
    {
      g_d3dpp.BackBufferWidth = g_ResizeWidth;
      g_d3dpp.BackBufferHeight = g_ResizeHeight;
      g_ResizeWidth = g_ResizeHeight = 0;
      ResetDevice();
    }

    // Start the Dear ImGui frame
    ImGui_ImplDX9_NewFrame();
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
      ImGuiImage::Zoomable(texture, displaySize, &zoomState);
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
    ImGui::EndFrame();
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(
      static_cast<int>(clearColor.x*clearColor.w*255.0f),
      static_cast<int>(clearColor.y*clearColor.w*255.0f),
      static_cast<int>(clearColor.z*clearColor.w*255.0f),
      static_cast<int>(clearColor.w*255.0f));
    g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
      clear_col_dx, 1.0f, 0);
    if (g_pd3dDevice->BeginScene() >= 0)
    {
      ImGui::Render();
      ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
      g_pd3dDevice->EndScene();
    }
    HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
    if (result == D3DERR_DEVICELOST)
        g_DeviceLost = true;
  }

  // Cleanup
  if (texture)
    texture->Release();
  ImGui_ImplDX9_Shutdown();
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
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
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
