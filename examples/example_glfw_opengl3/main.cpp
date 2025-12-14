// Dear ImGui: standalone example application for GLFW + OpenGL 3, using
// programmable pipeline (GLFW is a cross-platform general purpose library
// for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation,
// etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs
// - Introduction, links and more at the top of imgui.cpp

#include "../../imgui_zoomable_image.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
# include <GLES2/gl2.h>
#endif

#include <stdio.h>
#include <vector>
#include <cinttypes>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
      return 1;

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100 (WebGL 1.0)
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
  const char* glsl_version = "#version 300 es";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

  // Create window with graphics context
  float mainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(
    glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
  GLFWwindow* window = glfwCreateWindow(
    static_cast<int>(1280 * mainScale),
    static_cast<int>(800 * mainScale),
    "ImGui Zoomable Image GLFW+OpenGL3 example",
    nullptr, nullptr);
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(mainScale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi = mainScale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

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
  while (!glfwWindowShouldClose(window))
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    //   your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    //   data to your main application, or clear/overwrite your copy of the
    //   keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them
    // from your application based on those two flags.
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
    {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
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
    int displayWidth, displayHeight;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w,
                 clearColor.z * clearColor.w, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  glDeleteTextures(1, &textureId);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
