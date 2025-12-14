cmake_minimum_required(VERSION 3.31.6)

project(imgui_zoomable_image)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
      "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
      "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG}
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(LINUX TRUE)
endif()

if(MSVC)
  add_compile_options(/permissive-)
endif()

option(USE_GLFW "Enable GLFW backend" ON)
if (WIN32)
  option(USE_WIN32 "Enable Win32 backend" ON)
endif(WIN32)

option(USE_OPENGL3 "Enable OpenGL3 backend" ON)
if (WIN32)
  option(USE_DIRECTX9 "Enable DirectX9 backend" ON)
  option(USE_DIRECTX10 "Enable DirectX10 backend" ON)
  option(USE_DIRECTX11 "Enable DirectX11 backend" ON)
  option(USE_DIRECTX12 "Enable DirectX12 backend" ON)
endif(WIN32)

set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/3rd-party/imgui)

if(USE_GLFW)
  message(STATUS "GLFW backend enabled")
  set(GLFW_DIR ${PROJECT_SOURCE_DIR}/3rd-party/glfw)
endif(USE_GLFW)

if (USE_WIN32)
  message(STATUS "Win32 backend enabled")
endif(USE_WIN32)

if(USE_OPENGL3)
  message(STATUS "OpenGL3 backend enabled")
endif(USE_OPENGL3)
if (USE_DIRECTX9)
  message(STATUS "DirectX9 backend enabled")
endif(USE_DIRECTX9)
if (USE_DIRECTX10)
  message(STATUS "DirectX10 backend enabled")
endif(USE_DIRECTX10)
if (USE_DIRECTX11)
  message(STATUS "DirectX11 backend enabled")
endif(USE_DIRECTX11)
if (USE_DIRECTX12)
  message(STATUS "DirectX12 backend enabled")
endif(USE_DIRECTX12)

add_subdirectory(3rd-party)
add_subdirectory(examples)
