#include "gpu/gpu_frame.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "gpu.h"
#include "gpu/gpu_pipeline.h"
#include "gpu/gpu_swapchain.h"
#include "gpu_device.h"

// REGION: COMMAND POOL

void aso_vk_create_command_pool(aso_vk_frame *frame, const aso_vk_device *device) {
  ASSERT(frame != NULL);
  ASSERT(device != NULL);

  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // reset command buffers every frame
  pool_info.queueFamilyIndex = device->queue_families.graphics_family;

  ASO_VK_CHECK(vkCreateCommandPool(device->device, &pool_info, NULL, &frame->command_pool), "Failed to create command pool");
}

// REGION: COMMAND BUFFER

void aso_vk_create_command_buffers(aso_vk_frame *frame, const aso_vk_device *device) {
  ASSERT (frame != NULL);
  ASSERT (device != NULL);
  
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = frame->command_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = ASO_VK_FRAMES_IN_FLIGHT;

  ASO_VK_CHECK(vkAllocateCommandBuffers(device->device, &alloc_info, frame->command_buffers), "Failed to allocate command buffers");
}

void aso_vk_record_command_buffer(aso_vk_frame *frame, const aso_vk_swapchain *swapchain, const aso_vk_pipeline *pipeline, u32 image_index) {
  ASSERT(frame != NULL);
  ASSERT(swapchain != NULL);

  u32 f = frame->current;

  // begin

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0; // optional
  begin_info.pInheritanceInfo = NULL; // optional

  ASO_VK_CHECK(vkBeginCommandBuffer(frame->command_buffers[f], &begin_info), "Failed to begin recording command buffer");

  // start render pass

  VkRenderPassBeginInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = swapchain->render_pass;
  render_pass_info.framebuffer = swapchain->framebuffers[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = swapchain->extent;
  
  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(frame->command_buffers[f], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  // bind pipeline

  vkCmdBindPipeline(frame->command_buffers[f], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->graphics_pipeline);

  // define viewport and scissor as they were set to dynamic

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) swapchain->extent.width;
  viewport.height = (float) swapchain->extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(frame->command_buffers[f], 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchain->extent;
  vkCmdSetScissor(frame->command_buffers[f], 0, 1, &scissor);

  // draw!

  vkCmdDraw(frame->command_buffers[f], 3, 1, 0, 0);

  vkCmdEndRenderPass(frame->command_buffers[f]);

  ASO_VK_CHECK(vkEndCommandBuffer(frame->command_buffers[f]), "Failed to record command buffer");
}

void aso_vk_create_sync_objects(aso_vk_frame *frame, const aso_vk_device *device) {
  ASSERT(frame != NULL);
  ASSERT(device != NULL);
  
  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so we dont block forever on first frame

  for (size_t i = 0; i < ASO_VK_FRAMES_IN_FLIGHT; i++) {
    ASO_VK_CHECK(vkCreateSemaphore(device->device, &semaphore_info, NULL, &frame->image_available_semaphores[i]), "Failed to create semaphore");
    ASO_VK_CHECK(vkCreateFence(device->device, &fence_info, NULL, &frame->in_flight_fences[i]), "Failed to create fence");
  }
  // one render_finished_semaphore per image
  // when we get an image back from the swap chain, we are implicitly guaranteed that
  // the presentation engine has completed the work and the semaphore can be reused
  // https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
  for (size_t i = 0; i < ASO_VK_SWAP_CHAIN_MAX_IMAGES; i++) {
    ASO_VK_CHECK(vkCreateSemaphore(device->device, &semaphore_info, NULL, &frame->render_finished_semaphores[i]), "Failed to create semaphore");
  }
}
