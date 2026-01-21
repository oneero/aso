#ifndef ASO_GFX_H
#define ASO_GFX_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "core.h"
#include "mem.h"

#ifdef ASO_VULKAN_VALIDATION_LAYERS
static bool aso_enable_vulkan_validation_layers = true;
#else
static bool aso_enable_vulkan_validation_layers = false;
#endif

#define ASO_VULKAN_VALIDATION_LAYER_COUNT 1

// vulkan context
struct aso_vulkan_ctx {
  VkInstance instance;
};

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx);
void aso_create_vulkan_instance(VkInstance *instance);
void aso_cleanup_vulkan(aso_vulkan_ctx *vulkan_ctx);

VkExtensionProperties *aso_get_available_vulkan_extensions(aso_arena *arena,
                                                           u32 *count);
char const *const *aso_get_vulkan_extensions(u32 *count);
VkLayerProperties *aso_get_available_vulkan_layers(aso_arena *arena,
                                                   u32 *count);
char const *const *aso_get_vulkan_layers(u32 *count);
bool aso_check_vulkan_validation_layer_support(
    VkLayerProperties *available_layers, u32 count);
#endif // ASO_GFX_H
