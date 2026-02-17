#ifndef ASO_GPU_DEVICE_H
#define ASO_GPU_DEVICE_H

#include <vulkan/vulkan.h>

#include "base.h"
#include "gpu_config.h"
#include "mem.h"

struct aso_vk_queue_families
{
  u32  graphics_family;
  u32  present_family;
  bool has_graphics_family;
  bool has_present_family;
};

struct aso_vk_surface_details
{
  VkSurfaceFormatKHR formats[ASO_VK_MAX_SURFACE_FORMATS];
  u32                formats_count;
  VkPresentModeKHR   present_modes[ASO_VK_MAX_PRESENT_MODES];
  u32                present_modes_count;
};

struct aso_vk_device
{
  VkInstance             instance;
  VkPhysicalDevice       physical_device;
  VkDevice               device;
  VkSurfaceKHR           surface;
  VkQueue                graphics_queue;
  VkQueue                presentation_queue;
  aso_vk_queue_families  queue_families;
  aso_vk_surface_details surface_details;

  bool                   use_validation;
};

void                   aso_vk_device_init(aso_arena *scratch, aso_vk_device *device);
void                   aso_vk_device_cleanup(aso_vk_device *device);

void                   aso_vk_instance_init(aso_arena *scratch, aso_vk_device *device);
void                   aso_vk_select_physical_device(aso_arena *scratch, aso_vk_device *device);
void                   aso_vk_create_logical_device(aso_vk_device *device);

// layers
char const *const     *aso_vk_resolve_layers(aso_arena *scratch, u32 *count, bool *use_validation);
VkLayerProperties     *aso_vk_get_available_layers(aso_arena *scratch, u32 *count);
bool                   aso_vk_check_validation_layer_support(VkLayerProperties *available_layers, u32 count);

// extensions
char const *const     *aso_vk_resolve_extensions(aso_arena *scratch, u32 *count, bool use_validation);
VkExtensionProperties *aso_vk_get_available_extensions(aso_arena *scratch, u32 *count);
bool                   aso_vk_check_device_extension_support(aso_arena *scratch, VkPhysicalDevice physical_device);

// other cached details
aso_vk_queue_families  aso_vk_get_queue_families(aso_arena *scratch, VkPhysicalDevice physical_device, VkSurfaceKHR surface);
aso_vk_surface_details aso_vk_get_surface_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

#endif // ASO_GPU_DEVICE_H
