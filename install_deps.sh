#!/bin/bash
set -e

packages=(
  base-devel

  # Vulkan
  vulkan-headers
  vulkan-tools
  vulkan-validation-layers

  # Shaders
  shaderc
  spirv-tools
  glslang

  # SDL3
  sdl3

  # Wayland
  wayland
  wayland-protocols
  libxkbcommon

  # Build tools
  cmake
  gcc
  clang
  ninja
  pkg-config

  # Development
  gdb
  # renderdoc
  # tracy
)

echo "Installing dependencies..."
sudo pacman -Sp --needed "${packages[@]}"
sudo pacman -S --needed "${packages[@]}"
echo "Done."
