{
  description = "Vulkan renderer with SDL3 on Wayland";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            # Vulkan
            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
            vulkan-tools

            # Shaders
            shaderc
            spirv-tools
            glslang
            shader-slang

            # SDL3
            sdl3

            # Math
            # glm

            # Wayland (SDL3 should pull these in, but explicit doesn't hurt)
            wayland
            wayland-protocols
            libxkbcommon

            # Build tools
            cmake
            # pkg-config
            gcc
            # ninja

            # Development tools
            gdb
            renderdoc
            tracy

            # Image loading (optional)
            # stb
          ];

          shellHook = ''
            echo "🌋 Vulkan + SDL3 + Wayland Development Environment"
            echo "=================================================="
            echo ""
            echo "Versions:"
            echo "  • Vulkan: $(pkg-config --modversion vulkan)"
            echo "  • SDL3: $(pkg-config --modversion sdl3)"
            echo ""
            echo "Environment:"
            echo "  • Display: $WAYLAND_DISPLAY"
            echo "  • Session: $XDG_SESSION_TYPE"
            echo ""

            # Verify Vulkan
            if vulkaninfo --summary &>/dev/null; then
              echo "✓ Vulkan is working!"
              vulkaninfo --summary | grep -E "GPU|deviceName|driverInfo" | head -5
            else
              echo "⚠ Warning: vulkaninfo failed"
            fi
            echo ""
            echo "Ready to build! Try: cmake -B build -G Ninja && cmake --build build"
          '';

          # Prefer Wayland for SDL3
          SDL_VIDEODRIVER = "wayland";

          # Vulkan validation layers
          VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
        };
      }
    );
}
