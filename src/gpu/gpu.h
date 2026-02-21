#ifndef ASO_GPU_H
#define ASO_GPU_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "mem.h"

#include "gpu_device.h"
#include "gpu_swapchain.h"
#include "gpu_pipeline.h"
#include "gpu_frame.h"

// vulkan context
struct aso_vk_ctx
{
  // aso_arena    *frame_arena;
  bool             window_resized;

  aso_vk_device    device;
  aso_vk_swapchain swapchain;
  aso_vk_pipeline  pipeline;
  aso_vk_frame     frame;
};

// main api
void                            aso_vk_init(aso_arena *scratch, aso_vk_ctx *ctx);
void                            aso_vk_draw_frame(aso_vk_ctx *ctx);
void                            aso_vk_cleanup(aso_vk_ctx *ctx);

// REGION: HELPER MACROS

// performs call and compares result with expected (defaults to VK_SUCCESS)
// on false outputs to log and exit(1)
#define ASO_VK_CHECK(call, msg) ASO_VK_CHECK_EX(call, VK_SUCCESS, msg)
#define ASO_VK_CHECK_EX(call, expected, msg)                      \
  STMNT(                                                          \
    VkResult result = call;                                       \
    if (result != expected) {                                     \
      LOG_ERROR("VULKAN ERROR\n"                                  \
              " %s\n"                                             \
              " %s:%d: %s != %s (got %d)\n",                      \
              msg, __FILE__, __LINE__, #call, #expected, result); \
      DEBUG_TRAP();                                               \
    }                                                             \
  )

#endif // ASO_GPU_H
