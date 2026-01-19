#ifndef ASO_GFX_H
#define ASO_GFX_H

#include "core.h"
#include <vulkan/vulkan.h>

// vulkan context
struct aso_vulkan_ctx {
  VkInstance instance;
};

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx);
void aso_create_vulkan_instance(VkInstance *instance);
char const * const * aso_get_vulkan_extensions(u32* count);

#endif // ASO_GFX_H
