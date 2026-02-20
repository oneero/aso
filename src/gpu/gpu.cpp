#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "mem.h"
#include "gpu.h"
#include "gpu_device.h"
#include "gpu_swapchain.h"
#include "gpu_pipeline.h"
#include "gpu_frame.h"

void aso_vk_init(aso_arena *scratch, aso_vk_ctx *ctx) {
  size_t scratch_mark = scratch->offset;
  aso_vk_device_init(scratch, &ctx->device);
  scratch->offset = scratch_mark;
  aso_vk_swapchain_create(&ctx->swapchain, &ctx->device);
  aso_vk_swapchain_create_image_views(&ctx->swapchain, &ctx->device);
  aso_vk_swapchain_create_render_pass(&ctx->swapchain, &ctx->device);
  aso_vk_swapchain_create_framebuffers(&ctx->swapchain, &ctx->device);
  aso_vk_create_graphics_pipeline(scratch, &ctx->pipeline, ctx->device.device, ctx->swapchain.render_pass);
  scratch->offset = scratch_mark;

  // TODO: find a better place for this
  aso_vk_vertex vertices[] = {
      {.pos = {.x =  0.0f, .y = -0.5f}, .color = {.x = 1.0f, .y = 1.0f, .z = 1.0f}},
      {.pos = {.x =  0.5f, .y =  0.5f}, .color = {.x = 0.0f, .y = 1.0f, .z = 0.0f}},
      {.pos = {.x = -0.5f, .y =  0.5f}, .color = {.x = 0.0f, .y = 0.0f, .z = 1.0f}},
  };
  aso_vk_create_vertex_buffer(vertices, 3, &ctx->device, &ctx->pipeline);
  
  aso_vk_create_command_pool(&ctx->frame, &ctx->device);
  aso_vk_create_command_buffers(&ctx->frame, &ctx->device);
  aso_vk_create_sync_objects(&ctx->frame, &ctx->device);
}

// REGION: DRAW FRAME

void aso_vk_draw_frame(aso_vk_ctx *ctx) {
  ASSERT(ctx != NULL);

  u32 f = ctx->frame.current;

  // wait for previous queue submit to complete
  // NOTE: does not guarantee presentation engine has finished
  vkWaitForFences(ctx->device.device, 1, &ctx->frame.in_flight_fences[f], VK_TRUE, UINT64_MAX);

  u32 image_index;
  VkResult image_acquire_result = vkAcquireNextImageKHR(ctx->device.device, ctx->swapchain.handle, UINT64_MAX, ctx->frame.image_available_semaphores[f], VK_NULL_HANDLE, &image_index);

  // check if we need to recreate swap chain due to changed extents etc
  if (image_acquire_result == VK_ERROR_OUT_OF_DATE_KHR ||
      image_acquire_result == VK_SUBOPTIMAL_KHR ||
      ctx->window_resized) {
    ctx->window_resized = false;
    D_LOG("Swap chain image out of date or resized when acquiring image");

    aso_vk_swapchain_recreate(&ctx->swapchain, &ctx->device);
    // need to recreate tainted semaphore
    // NOTE: recreate_swap_chain() above calls vkDeviceWaitIdle()
    vkDestroySemaphore(ctx->device.device, ctx->frame.image_available_semaphores[f], NULL);
    VkSemaphoreCreateInfo info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device.device, &info, NULL, &ctx->frame.image_available_semaphores[f]);

    return;
  }
  ASSERT (image_acquire_result == VK_SUCCESS);

  // reset fence after we know we will be using acquired image
  vkResetFences(ctx->device.device, 1, &ctx->frame.in_flight_fences[f]);
  
  vkResetCommandBuffer(ctx->frame.command_buffers[f], 0);
  aso_vk_record_command_buffer(ctx->frame.command_buffers[f], &ctx->swapchain, &ctx->pipeline, image_index);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &ctx->frame.image_available_semaphores[f];
  submit_info.pWaitDstStageMask = wait_stages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &ctx->frame.command_buffers[f];

  // use image_index here and per image semaphore so we know presentation has completed
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &ctx->frame.render_finished_semaphores[image_index];

  ASO_VK_CHECK(vkQueueSubmit(ctx->device.graphics_queue, 1, &submit_info, ctx->frame.in_flight_fences[f]), "Failed to submit draw command buffer");

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &ctx->frame.render_finished_semaphores[image_index];

  present_info.swapchainCount = 1;
  present_info.pSwapchains = &ctx->swapchain.handle;
  present_info.pImageIndices = &image_index;

  present_info.pResults = NULL; // optional

  VkResult present_result = vkQueuePresentKHR(ctx->device.presentation_queue, &present_info);
  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR ||
      ctx->window_resized) {
    D_LOG("Swap chain image out of date when presenting");
    ctx->window_resized = false;
    aso_vk_swapchain_recreate(&ctx->swapchain, &ctx->device);
  } else if (present_result != VK_SUCCESS) {
    LOG_ERROR("Failed to present swap chain image");
  }

  ctx->frame.current = (f + 1) % ASO_VK_FRAMES_IN_FLIGHT;
}

// REGION: CLEANUP

void aso_vk_cleanup(aso_vk_ctx *ctx) {
  vkDeviceWaitIdle(ctx->device.device);

  // sync objects, command pool and buffers
  aso_vk_frame_cleanup(&ctx->frame, &ctx->device);

  // pipeline and layout
  aso_vk_pipeline_cleanup(&ctx->pipeline, &ctx->device);
  
  // framebuffers, image views, swapchain
  aso_vk_swapchain_cleanup(&ctx->swapchain, &ctx->device);

  vkDestroyRenderPass(ctx->device.device, ctx->swapchain.render_pass, NULL);
  
  // device, surface and instance
  aso_vk_device_cleanup(&ctx->device);

  // TODO: aso_arena_destroy(ctx->frame_arena);
}
