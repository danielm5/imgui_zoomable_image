// Dear ImGui: standalone example application for Windows API + OpenGL

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// This is provided for completeness, however it is strongly recommended you use OpenGL with SDL or GLFW.

#include "../../imgui_zoomable_image.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#include <tchar.h>

#include <stdio.h>
#include <vector>
#include <cinttypes>

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
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
    sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr),
    nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
  ::RegisterClassExW(&wc);
  HWND hwnd = ::CreateWindowW(wc.lpszClassName,
    L"Dear ImGui Zoomable Image Win32+OpenGL3 Example", WS_OVERLAPPEDWINDOW,
    100, 100, (int)(1280 * main_scale), (int)(800 * main_scale),
    nullptr, nullptr, wc.hInstance, nullptr);

  // Initialize OpenGL
  if (!CreateDeviceWGL(hwnd, &g_MainWindow))
  {
    CleanupDeviceWGL(hwnd, &g_MainWindow);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 1;
  }
  wglMakeCurrent(g_MainWindow.hDC, g_hRC);

  // Show the window
  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

  // Setup Platform/Renderer backends
  ImGui_ImplWin32_InitForOpenGL(hwnd);
  ImGui_ImplOpenGL3_Init();

  // Create an OpenGL texture
  GLuint textureId = 0;
  const size_t width = 320, height = 240;
  {
    // Initialize OpenGL texture
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // make checkerboard pattern
    const size_t checkerSize = 20;
    std::vector<uint8_t> data(width * height * 4);
    for (size_t y = 0; y < height; ++y)
    {
      uint8_t* row = &data[y * width * 4];
      for (size_t x = 0; x < width; ++x)
      {
        const bool isWhite = ((x / checkerSize) % 2) == ((y / checkerSize) % 2);
        row[x * 4 + 0] = isWhite ? 200 : 50;
        row[x * 4 + 1] = isWhite ? 200 : 50;
        row[x * 4 + 2] = isWhite ? 200 : 50;
        row[x * 4 + 3] = 255;
      }
    }

    // Upload image data to texture
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      static_cast<GLsizei>(width),
      static_cast<GLsizei>(height),
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      data.data());

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
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
    if (::IsIconic(hwnd))
    {
      ::Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
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
      ImGuiImage::Zoomable(textureId, displaySize, &zoomState);
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
    glViewport(0, 0, g_Width, g_Height);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Present
    ::SwapBuffers(g_MainWindow.hDC);
  }

  // Cleanup
  glDeleteTextures(1, &textureId);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceWGL(hwnd, &g_MainWindow);
  wglDeleteContext(g_hRC);
  ::DestroyWindow(hwnd);
  ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

  return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
  HDC hDc = ::GetDC(hWnd);
  PIXELFORMATDESCRIPTOR pfd = { 0 };
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;

  const int pf = ::ChoosePixelFormat(hDc, &pfd);
  if (pf == 0)
    return false;
  if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
    return false;
  ::ReleaseDC(hWnd, hDc);

  data->hDC = ::GetDC(hWnd);
  if (!g_hRC)
    g_hRC = wglCreateContext(data->hDC);
  return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
  wglMakeCurrent(nullptr, nullptr);
  ::ReleaseDC(hWnd, data->hDC);
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
    if (wParam != SIZE_MINIMIZED)
    {
      g_Width = LOWORD(lParam);
      g_Height = HIWORD(lParam);
    }
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
