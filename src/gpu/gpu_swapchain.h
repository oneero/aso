#ifndef ASO_GPU_SWAPCHAIN_H
#define ASO_GPU_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "gpu_config.h"
#include "gpu_device.h"

struct aso_vk_swapchain
{
  VkSwapchainKHR handle;
  VkFormat       format;
  VkExtent2D     extent;
  VkImage        images[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  VkImageView    image_views[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  VkFramebuffer  framebuffers[ASO_VK_SWAP_CHAIN_MAX_IMAGES];
  u32            image_count;
  VkRenderPass   render_pass; // TODO: move/remove from here
};

void       aso_vk_create_swapchain(aso_vk_swapchain *swapchain, const aso_vk_device *device);
void       aso_vk_create_image_views(aso_vk_swapchain *swapchain, const aso_vk_device *device);
void       aso_vk_create_framebuffers(aso_vk_swapchain *swapchain, const aso_vk_device *device);
void       aso_vk_create_render_pass(aso_vk_swapchain *swapchain, const aso_vk_device *device);
void       aso_vk_recreate_swapchain(aso_vk_swapchain *swapchain, const aso_vk_device *device);
void       aso_vk_cleanup_swapchain(aso_vk_swapchain *swapchain, const aso_vk_device *device);
VkExtent2D aso_vk_get_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities);

#endif // ASO_GPU_SWAPCHAIN_H
