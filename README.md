**aso**
- a playground/sandbox for running, visualizing and playing with algorithms on GPUs.
- a learning platform for myself.

Interactive real-time simulations powered by GPU compute. Cellular automata, physarum, boids, alife. The idea is to create a new tool/toy for myself and learn new things along the way.

## State

aso is very early in its development.

Initially aso will simply be a re-implementation of my previous compute shader sandbox, which was built on top of [bgfx](https://github.com/bkaradzic/bgfx). This time I will be using Vulkan and SDL3 in order to learn new things and hone old skills.

## Roadmap

- [x] triangle
- [x] ~switch to dynamic rendering~
- [x] indexed quad
- [x] uniforms
- [ ] textures
- [ ] compute shader support
- [ ] cca/boids examples
- [ ] gui
- [ ] user defined shaders and params 
- [ ] hot reloads and file watching
- [ ] algorithm exploration tools

## Running

Linux is currently the main target platform and the only one I am testing on. I will try to support Windows as well and have it tested when the project matures. Possible MacOS support will be explored later.

Currently the only way to run is to build it from source. See below.

## Development

You can replicate my arch-based development environment with the install_deps.sh script or adapt it for your environment.

Distrobox is likely the easiest way to setup a containerized environment in order to not contaminate your system configuration.

1. Ensure your system can run vulkaninfo / vkcube first.
2. Setup distrobox environment:
```bash
distrobox create --name aso-dev --image archlinux:latest --nvidia # leave out if AMD
distrobox enter aso-dev
./install_deps.sh
```

## Contributions

The project is too early in development for me to consider accepting any contributions.
