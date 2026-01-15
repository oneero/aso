# aso
Vulkan sandbox

# env

You can find a list of dependencies in the install_deps.sh script. Distrobox is likely the easiest way to setup a containerized environment with the necessary dependencies without contaminating your system configuration.

1. Ensure your system can run vulkaninfo / vkcube first.
2. Setup distrobox environment:
```bash
distrobox create --name aso-dev --image archlinux:latest --nvidia # leave out if AMD
distrobox enter aso-dev
./install_deps.sh
```
