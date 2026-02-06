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
#define ASO_VULKAN_DEVICE_EXTENSION_COUNT 1

// vulkan context
struct aso_vulkan_ctx {
  aso_arena *arena;
  // these are pointer sized handles, Vulkan manages the lifetime
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue presentation_queue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swap_chain;
  VkFormat swap_chain_format;
  VkExtent2D swap_chain_extent;
  VkImage *swap_chain_images;
  u32 swap_chain_images_count;
  VkImageView *swap_chain_image_views;
  u32 swap_chain_image_views_count;
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;
  VkFramebuffer *swap_chain_framebuffers;
  u32 swap_chain_framebuffers_count;
};

struct aso_vulkan_queue_family_indices {
  u32 graphics_family;
  u32 present_family;
  bool has_graphics_family;
  bool has_present_family;
};

struct aso_vulkan_swap_chain_support_details {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  u32 formats_count;
  VkPresentModeKHR *present_modes;
  u32 present_modes_count;
};

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx);

void aso_create_vulkan_instance(aso_vulkan_ctx *vulkan_ctx);
VkExtensionProperties* aso_get_available_vulkan_extensions(aso_arena *arena, u32 *count);
char const * const * aso_get_vulkan_extensions(u32 *count);
VkLayerProperties* aso_get_available_vulkan_layers(aso_arena *arena, u32 *count);
char const * const * aso_get_vulkan_layers(u32 *count);
bool aso_check_vulkan_validation_layer_support(VkLayerProperties *available_layers, u32 count);

void aso_select_physical_device(aso_vulkan_ctx *vulkan_ctx);
bool aso_is_device_suitable(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device);
aso_vulkan_queue_family_indices aso_get_vulkan_family_indices(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device);
bool aso_check_device_extension_support(aso_arena *arena, VkPhysicalDevice physical_device);
aso_vulkan_swap_chain_support_details aso_query_swap_chain_support(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device);

void aso_create_vulkan_logical_device(aso_vulkan_ctx *vulkan_ctx);

void aso_create_swap_chain(aso_vulkan_ctx *vulkan_ctx);
VkExtent2D aso_select_swap_extent(VkSurfaceCapabilitiesKHR *capabilities);

void aso_create_image_views(aso_vulkan_ctx *vulkan_ctx);
void aso_create_graphics_pipeline(aso_vulkan_ctx *vulkan_ctx);
VkShaderModule aso_create_shader_module(VkDevice device, u8 *shader_code, long code_size);

void aso_create_render_pass(aso_vulkan_ctx *vulkan_ctx);
void aso_create_framebuffers(aso_vulkan_ctx *vulkan_ctx);

void aso_cleanup_vulkan(aso_vulkan_ctx *vulkan_ctx);

// REGION: HELPER MACROS

// performs call and compares result with expected (defaults to VK_SUCCESS)
// on false outputs to log and exit(1)
#define VK_CHECK(call, msg) VK_CHECK_EX(call, VK_SUCCESS, msg)
#define VK_CHECK_EX(call, expected, msg) \
  do { \
    VkResult result = call; \
    if (result != expected) { \
      aso_log("VULKAN ERROR\n" \
              " %s\n" \
              " %s:%d: %s != %s (got %s)\n", \
              msg, __FILE__, __LINE__, #call, #expected, result); \
      exit(1); \
    } \
  } while(0)

#endif // ASO_GFX_H
