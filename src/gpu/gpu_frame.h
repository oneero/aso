#ifndef ASO_GPU_FRAME_H
#define ASO_GPU_FRAME_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "gpu_device.h"
#include "gpu_pipeline.h"
#include "gpu_scene.h"
#include "gpu_swapchain.h"
#include "gpu_config.h"

struct aso_vk_frame
{
  u32             current;
  VkCommandPool   command_pool;
  VkCommandBuffer command_buffers[ASO_VK_FRAMES_IN_FLIGHT];
  VkSemaphore     image_available_semaphores[ASO_VK_FRAMES_IN_FLIGHT];
  VkSemaphore     render_finished_semaphores[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  VkFence         in_flight_fences[ASO_VK_FRAMES_IN_FLIGHT];
};

void aso_vk_create_command_pool(aso_vk_frame *frame, const aso_vk_device *device);
void aso_vk_create_command_buffers(aso_vk_frame *frame, const aso_vk_device *device);
void aso_vk_record_command_buffer(VkCommandBuffer buffer, const aso_vk_swapchain *swapchain, const aso_vk_pipeline *pipeline, const aso_vk_scene *scene, u32 image_index);
void aso_vk_create_sync_objects(aso_vk_frame *frame, const aso_vk_device *device);

void aso_vk_frame_cleanup(aso_vk_frame *frame, const aso_vk_device *device);

#endif // ASO_GPU_FRAME_H
