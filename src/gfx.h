#ifndef ASO_GFX_H
#define ASO_GFX_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "core.h"
#include "mem.h"

#ifdef ASO_VK_VALIDATION_LAYERS
static bool aso_vk_enable_validation_layers = true;
#else
static bool aso_vk_enable_validation_layers = false;
#endif

#define ASO_VK_VALIDATION_LAYER_COUNT 1
#define ASO_VK_DEVICE_EXTENSION_COUNT 1
#define ASO_VK_FRAMES_IN_FLIGHT 2
#define ASO_VK_SWAP_CHAIN_MAX_IMAGES 4
#define ASO_VK_MAX_SURFACE_FORMATS 32
#define ASO_VK_MAX_PRESENT_MODES 8

struct aso_vk_queue_families
{
  u32  graphics_family;
  u32  present_family;
  bool has_graphics_family;
  bool has_present_family;
};

struct aso_vk_surface_details
{
  VkSurfaceFormatKHR       formats[ASO_VK_MAX_SURFACE_FORMATS];
  u32                      formats_count;
  VkPresentModeKHR         present_modes[ASO_VK_MAX_PRESENT_MODES];
  u32                      present_modes_count;
};

// vulkan context
// mostly vulkan handles
struct aso_vk_ctx
{
  // aso_arena          *frame_arena;
  bool                   window_resized;

  // configuration
  VkInstance             instance;
  VkPhysicalDevice       physical_device;
  VkDevice               device;
  VkSurfaceKHR           surface;
  VkQueue                graphics_queue;
  VkQueue                presentation_queue;
  aso_vk_queue_families  queue_families;
  aso_vk_surface_details surface_details;

  // swap chain
  VkSwapchainKHR         swap_chain;
  VkFormat               swap_chain_format;
  VkExtent2D             swap_chain_extent;
  VkImage                swap_chain_images[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  u32                    swap_chain_images_count;
  VkImageView            swap_chain_image_views[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  u32                    swap_chain_image_views_count;
  VkFramebuffer          swap_chain_framebuffers[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  u32                    swap_chain_framebuffers_count;

  // pipeline
  VkRenderPass           render_pass;
  VkPipelineLayout       pipeline_layout;
  VkPipeline             graphics_pipeline;

  // frame data
  u32                    frame;
  VkCommandPool          command_pool;
  VkCommandBuffer        command_buffers[ASO_VK_FRAMES_IN_FLIGHT];
  VkSemaphore            image_available_semaphores[ASO_VK_FRAMES_IN_FLIGHT];
  VkSemaphore            render_finished_semaphores[ASO_VK_FRAMES_IN_FLIGHT];
  VkFence                in_flight_fences[ASO_VK_FRAMES_IN_FLIGHT];
};

// main api
void                      aso_vk_init(aso_arena *scratch, aso_vk_ctx *ctx);
void                      aso_vk_draw_frame(aso_vk_ctx *ctx);
void                      aso_vk_cleanup(aso_vk_ctx *ctx);

// device configuration
void                      aso_vk_create_instance(aso_arena *scratch, aso_vk_ctx *ctx);
void                      aso_vk_select_physical_device(aso_arena *scratch, aso_vk_ctx *ctx);
void                      aso_vk_create_logical_device(aso_vk_ctx *ctx);

VkExtensionProperties    *aso_vk_get_available_extensions(aso_arena *scratch, u32 *count);
VkLayerProperties        *aso_vk_get_available_layers(aso_arena *scratch, u32 *count);
aso_vk_queue_families     aso_vk_get_queue_families(aso_arena *scratch, VkPhysicalDevice physical_device, VkSurfaceKHR surface);
char const *const        *aso_vk_define_extensions(u32 *count);
char const *const        *aso_vk_define_layers(u32 *count);
bool                      aso_vk_check_validation_layer_support(VkLayerProperties *available_layers, u32 count);
bool                      aso_vk_check_device_extension_support(aso_arena *scratch, VkPhysicalDevice physical_device);

// swap chain
void                      aso_vk_create_swap_chain(aso_vk_ctx *ctx);
void                      aso_vk_recreate_swap_chain(aso_vk_ctx *ctx);
void                      aso_vk_cleanup_swap_chain(aso_vk_ctx *ctx);
void                      aso_vk_create_image_views(aso_vk_ctx *ctx);
void                      aso_vk_create_framebuffers(aso_vk_ctx *ctx);
VkExtent2D                aso_vk_get_swap_extent(VkSurfaceCapabilitiesKHR capabilities);
aso_vk_surface_details    aso_vk_get_surface_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

// pipeline
void                      aso_vk_create_graphics_pipeline(aso_arena *scratch, aso_vk_ctx *ctx);
void                      aso_vk_create_render_pass(aso_vk_ctx *ctx);
VkShaderModule            aso_vk_create_shader_module(VkDevice device, u8 *shader_code, long code_size);

// frame
void                      aso_vk_create_command_pool(aso_vk_ctx *ctx);
void                      aso_vk_create_command_buffers(aso_vk_ctx *ctx);
void                      aso_vk_record_command_buffer(aso_vk_ctx *ctx, u32 image_index);
void                      aso_vk_create_sync_objects(aso_vk_ctx *ctx);

// REGION: HELPER MACROS

// performs call and compares result with expected (defaults to VK_SUCCESS)
// on false outputs to log and exit(1)
#define ASO_VK_CHECK(call, msg) ASO_VK_CHECK_EX(call, VK_SUCCESS, msg)
#define ASO_VK_CHECK_EX(call, expected, msg)                      \
  do {                                                            \
    VkResult result = call;                                       \
    if (result != expected) {                                     \
      aso_log("VULKAN ERROR\n"                                    \
              " %s\n"                                             \
              " %s:%d: %s != %s (got %d)\n",                      \
              msg, __FILE__, __LINE__, #call, #expected, result); \
      exit(1);                                                    \
    }                                                             \
  } while (0)

#endif // ASO_GFX_H
