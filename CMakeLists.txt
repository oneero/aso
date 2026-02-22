cmake_minimum_required(VERSION 3.20)
project(aso
  VERSION
    0.1
  DESCRIPTION
    "Vulkan sandbox"
  LANGUAGES
    CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEBUG_ARENA "Arena debugging" ON)
option(USE_SANITIZERS "ASan/UBSan" ON)
option(USE_VALGRIND "Valgrind" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(DEBUG_ARENA)
  add_compile_definitions(ASO_DEBUG_ARENA)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BUILD_DEBUG=1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(BUILD_RELEASE=1)
endif()

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(aso
    src/main.cpp
    src/aso.cpp
    src/window.cpp
    src/input.cpp
    src/mem.cpp
    src/io.cpp
    src/gpu/gpu.cpp
    src/gpu/gpu_device.cpp
    src/gpu/gpu_swapchain.cpp
    src/gpu/gpu_pipeline.cpp
    src/gpu/gpu_frame.cpp
    src/gpu/gpu_scene.cpp
)

target_include_directories(aso PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_definitions(aso PRIVATE ASO_VK_VALIDATION_LAYERS)

target_compile_options(aso PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wconversion
  -Wshadow
  -Werror
  -fno-exceptions
  -fno-rtti
  -Wno-unused-parameter
  -Wno-missing-designated-field-initializers
  -Wno-gnu-anonymous-struct
)

if(USE_VALGRIND)
  set(USE_SANITIZERS OFF)
  message(STATUS "Valgrind build: sanitizers disabled")
endif()

if(USE_SANITIZERS)
    target_compile_options(aso PRIVATE -fsanitize=address -fsanitize=undefined)
    target_link_options(aso PRIVATE -fsanitize=address -fsanitize=undefined)
    message(STATUS "Sanitizers enabled")
endif()

target_link_libraries(aso PRIVATE Vulkan::Vulkan SDL3::SDL3)
