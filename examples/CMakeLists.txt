
if (USE_GLFW)
  if (USE_OPENGL3)
    add_subdirectory(example_glfw_opengl3)
  endif(USE_OPENGL3)
endif(USE_GLFW)

if (USE_WIN32)
  if (USE_OPENGL3)
    add_subdirectory(example_win32_opengl3)
  endif(USE_OPENGL3)
endif(USE_WIN32)
