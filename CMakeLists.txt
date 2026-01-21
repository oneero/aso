cmake_minimum_required(VERSION 3.20)
project(aso
  VERSION
    0.1
  DESCRIPTION
    "Vulkan sandbox"
  LANGUAGES
    CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")

find_package(Vulkan REQUIRED)
find_package(SDL3 REQUIRED)

add_compile_definitions(ASO_VULKAN_VALIDATION_LAYERS)

add_executable(aso
    src/main.cpp
    src/aso.cpp
    src/window.cpp
    src/input.cpp
    src/gfx.cpp
    src/mem.cpp
)

#add_executable(aso src/simple.cpp)

target_link_libraries(aso PRIVATE Vulkan::Vulkan SDL3::SDL3)
