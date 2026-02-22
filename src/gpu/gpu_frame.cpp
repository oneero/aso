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

  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // reset command buffers every frame
    .queueFamilyIndex = device->queue_families.graphics_family,
  };

  ASO_VK_CHECK(vkCreateCommandPool(device->device, &pool_info, NULL, &frame->command_pool), "Failed to create command pool");
}

// REGION: COMMAND BUFFER

void aso_vk_create_command_buffers(aso_vk_frame *frame, const aso_vk_device *device) {
  ASSERT (frame != NULL);
  ASSERT (device != NULL);
  
  VkCommandBufferAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = frame->command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = ASO_VK_FRAMES_IN_FLIGHT,
  };

  ASO_VK_CHECK(vkAllocateCommandBuffers(device->device, &alloc_info, frame->command_buffers), "Failed to allocate command buffers");
}

void aso_vk_record_command_buffer(VkCommandBuffer buffer, const aso_vk_swapchain *swapchain, const aso_vk_pipeline *pipeline, const aso_vk_scene *scene, u32 image_index) {
  ASSERT(buffer != NULL);
  ASSERT(swapchain != NULL);

  // begin

  VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0, // optional
    .pInheritanceInfo = NULL, // optional
  };

  ASO_VK_CHECK(vkBeginCommandBuffer(buffer, &begin_info), "Failed to begin recording command buffer");

  // start render pass

  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderPassBeginInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = swapchain->render_pass,
    .framebuffer = swapchain->framebuffers[image_index],
    .renderArea = {
      .offset = {0, 0},
      .extent = swapchain->extent,
    },

    .clearValueCount = 1,
    .pClearValues = &clear_color,
  };

  vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  // bind pipeline

  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->graphics_pipeline);

  VkBuffer vertex_buffers[] = { scene->vertex_buffer };
  VkDeviceSize offsets[] = { 0 };
  vkCmdBindVertexBuffers(buffer, 0, 1, vertex_buffers, offsets);

  vkCmdBindIndexBuffer(buffer, scene->index_buffer, 0, VK_INDEX_TYPE_UINT16);

  // define viewport and scissor as they were set to dynamic

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float) swapchain->extent.width,
    .height = (float) swapchain->extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };

  vkCmdSetViewport(buffer, 0, 1, &viewport);

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = swapchain->extent,
  };

  vkCmdSetScissor(buffer, 0, 1, &scissor);

  // draw!

  //vkCmdDraw(buffer, pipeline->vertex_count, 1, 0, 0);
  vkCmdDrawIndexed(buffer, scene->index_count, 1, 0, 0, 0);
  vkCmdEndRenderPass(buffer);

  ASO_VK_CHECK(vkEndCommandBuffer(buffer), "Failed to record command buffer");
}

void aso_vk_create_sync_objects(aso_vk_frame *frame, const aso_vk_device *device) {
  ASSERT(frame != NULL);
  ASSERT(device != NULL);
  
  VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT, // start signaled so we dont block forever on first frame
  };

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

void aso_vk_frame_cleanup(aso_vk_frame *frame, const aso_vk_device *device) {
  ASSERT(frame != NULL);
  ASSERT(device != NULL);

  for (size_t i = 0; i < ASO_VK_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device->device, frame->image_available_semaphores[i], NULL);
    vkDestroyFence(device->device, frame->in_flight_fences[i], NULL);
  }
  for (size_t i = 0; i < ASO_VK_SWAP_CHAIN_MAX_IMAGES; i++) {
    vkDestroySemaphore(device->device, frame->render_finished_semaphores[i], NULL);
  }

  // NOTE: Destroying command pool will implicitly free command buffers
  vkDestroyCommandPool(device->device, frame->command_pool, NULL);
}
