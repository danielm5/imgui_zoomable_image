
# GLFW #######################################################################
if (USE_GLFW_GL3)
  if (LINUX)
    set(GLFW_BUILD_WAYLAND 0)
    set(GLFW_BUILD_X11 1)
  endif()
  add_subdirectory(glfw)
endif(USE_GLFW_GL3)

# Dear ImGui #################################################################

add_library(imgui STATIC
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
)

target_include_directories(imgui PUBLIC imgui imgui/backends imgui/misc/cpp)

if (USE_GLFW_GL3)
  target_sources(imgui PRIVATE
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
  )

  target_compile_definitions(imgui
    PUBLIC
      -DUSE_GLFW_GL3
  )

  target_link_libraries(imgui PUBLIC glfw)

  if (WIN32)
    target_link_libraries(imgui PUBLIC "opengl32")
  elseif(APPLE)
    target_link_libraries(imgui PUBLIC "-framework OpenGL")
  elseif(LINUX)
    target_link_libraries(imgui PUBLIC "-lGL")
  endif()

endif(USE_GLFW_GL3)
