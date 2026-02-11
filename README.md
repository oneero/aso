aso is a WIP re-implementation of my earlier compute shader sandbox/playground, but this time with Vulkan and SDL3. It is still very early in the development process. 

# high level goals

- [x] triangle
- [ ] textured quad with necessary shader buffers
- [ ] compute shader support
- [ ] cca/boids examples
- [ ] gui
- [ ] algorithm exploration tools

# env

Linux is currently the main target platform, although I'll try to support Windows as well.

You can find a list of packages used for development in the install_deps.sh script.

Distrobox is likely the easiest way to setup a containerized environment with the necessary dependencies without contaminating your system configuration.

1. Ensure your system can run vulkaninfo / vkcube first.
2. Setup distrobox environment:
```bash
distrobox create --name aso-dev --image archlinux:latest --nvidia # leave out if AMD
distrobox enter aso-dev
./install_deps.sh
```
