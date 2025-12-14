
# GLFW #######################################################################
if (USE_GLFW)
  if (LINUX)
    set(GLFW_BUILD_WAYLAND 0)
    set(GLFW_BUILD_X11 1)
  endif()
  add_subdirectory(glfw)
endif(USE_GLFW)

# Dear ImGui #################################################################

add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
)

target_include_directories(imgui PUBLIC imgui imgui/backends imgui/misc/cpp)

if (USE_GLFW)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_glfw.cpp)
  target_link_libraries(imgui PUBLIC glfw)
endif(USE_GLFW)

if (USE_WIN32)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_win32.cpp)
endif(USE_WIN32)

if (USE_OPENGL3)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_opengl3.cpp)
  if (WIN32)
    target_link_libraries(imgui PUBLIC "opengl32")
  elseif(APPLE)
    target_link_libraries(imgui PUBLIC "-framework OpenGL")
  elseif(LINUX)
    target_link_libraries(imgui PUBLIC "-lGL")
  endif()
endif(USE_OPENGL3)

if (USE_DIRECTX9)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_dx9.cpp)
  target_link_libraries(imgui PUBLIC d3d9)
endif(USE_DIRECTX9)

if (USE_DIRECTX10)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_dx10.cpp)
  target_link_libraries(imgui PUBLIC d3d10)
endif(USE_DIRECTX10)

if (USE_DIRECTX11)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_dx11.cpp)
  target_link_libraries(imgui PUBLIC d3d11)
endif(USE_DIRECTX11)

if (USE_DIRECTX12)
  target_sources(imgui PRIVATE imgui/backends/imgui_impl_dx12.cpp)
  target_link_libraries(imgui PUBLIC d3d12 dxgi)
endif(USE_DIRECTX12)
