#ifndef ASO_GFX_H
#define ASO_GFX_H

#include <vulkan/vulkan.h>

// vulkan context
struct aso_vulkan_ctx {
  VkInstance instance;
};

void aso_init_vulkan();
void aso_create_vulkan_instance();

#endif // ASO_GFX_H
