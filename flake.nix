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

            # Wayland
            wayland
            wayland-protocols
            libxkbcommon

            # Build tools
            cmake
            # pkg-config
            gcc
            ninja

            # Development tools
            gdb
            renderdoc
            tracy

            # Image loading (optional)
            # stb
          ];

          # Prefer Wayland for SDL3
          SDL_VIDEODRIVER = "wayland";

          # Vulkan validation layers
          VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
        };
      }
    );
}
