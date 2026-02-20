#include "gpu/gpu_swapchain.h"
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "gpu.h"
#include "gpu/gpu_device.h"

void aso_vk_swapchain_create(aso_vk_swapchain *swapchain, const aso_vk_device *device) {
  ASSERT(swapchain != NULL);
  ASSERT(device != NULL);

  D_LOG("Creating swap chain..");

  VkSurfaceCapabilitiesKHR capabilities = {};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, device->surface, &capabilities);
  
  const aso_vk_surface_details *details = &device->surface_details;

  // surface format
  VkSurfaceFormatKHR surface_format = details->formats[0];
  for (u32 i = 0; i < details->formats_count; i++) {
    if (details->formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && details->formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surface_format = details->formats[i];
      break;
    }
  }

  // present mode
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (u32 i = 0; i < details->present_modes_count; i++) {
    if (details->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }
  
  // extent
  VkExtent2D extent = aso_vk_swapchain_get_extent(capabilities);
  D_LOG(" Extent: %d x %d", extent.width, extent.height);

  u32 image_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
    image_count = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = device->surface,
    .minImageCount = image_count,
    .imageFormat = surface_format.format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
  };

  u32 queue_family_indices[] = { device->queue_families.graphics_family, device->queue_families.present_family };

  // if the queue families are different, we use concurrent mode to share images
  // NOTE: exclusive mode is ideal but requires explicit ownership transfers
  if (device->queue_families.graphics_family != device->queue_families.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2; // TODO: this should not be hardcoded here
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  ASO_VK_CHECK(vkCreateSwapchainKHR(device->device, &create_info, NULL, &swapchain->handle), "Failed to create swap chain");

  // retrieve swap chain image handles
  // we store in fixed sized arrays so we need to check the count
  u32 image_handle_count = 0;
  vkGetSwapchainImagesKHR(device->device, swapchain->handle, &image_handle_count, NULL);
  ASSERT(image_handle_count <= ASO_VK_SWAP_CHAIN_MAX_IMAGES);
  swapchain->image_count = image_handle_count;
  vkGetSwapchainImagesKHR(device->device, swapchain->handle, &swapchain->image_count, swapchain->images);

  // store format and extent for later use
  swapchain->format = surface_format.format;
  swapchain->extent = extent;
}

void aso_vk_swapchain_recreate(aso_vk_swapchain *swapchain, const aso_vk_device *device) {
  ASSERT(swapchain != NULL);
  ASSERT(device != NULL);

  // check if the window is minimized
  int width, heigth;
  aso_get_window_size(&g_ctx->window, &width, &heigth);
  while (width == 0 || heigth == 0) {
    aso_wait_for_sdl_event(NULL);
    aso_get_window_size(&g_ctx->window, &width, &heigth);
  }

  vkDeviceWaitIdle(device->device);

  aso_vk_swapchain_cleanup(swapchain, device);
  aso_vk_swapchain_create(swapchain, device);
  aso_vk_swapchain_create_image_views(swapchain, device);
  aso_vk_swapchain_create_framebuffers(swapchain, device);
}

VkExtent2D aso_vk_swapchain_get_extent(VkSurfaceCapabilitiesKHR capabilities) {
  // check current extents for special value
  // -> go with already set extent
  // -> special value set: set swap extent here and the surface will conform
  if (capabilities.currentExtent.width != 0xFFFFFFFF) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    aso_get_window_size(&g_ctx->window, &width, &height);
    D_LOG("Window extent: %dx%d", width, height);
    VkExtent2D extent = {
      CLAMP((u32)width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      CLAMP((u32)height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };

    return extent;
  }
}

// REGION: IMAGE VIEWS

void aso_vk_swapchain_create_image_views(aso_vk_swapchain *swapchain, const aso_vk_device *device) {
  ASSERT(swapchain != NULL);
  ASSERT(device->device != NULL);
  
  for (size_t i = 0; i < swapchain->image_count; i++) {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swapchain->images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = swapchain->format,
      .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY, },
      
      // subresourceRange describes image view usage details
      .subresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };
    ASO_VK_CHECK(vkCreateImageView(device->device, &create_info, NULL, &swapchain->image_views[i]), "Failed to create image view");
  }
}

// REGION: FRAMEBUFFERS

void aso_vk_swapchain_create_framebuffers(aso_vk_swapchain *swapchain, const aso_vk_device *device) {
  ASSERT(swapchain != NULL);
  ASSERT(device->device != NULL);

  // we create a framebuffer for each imageview in the swap chain
  for (size_t i = 0; i < swapchain->image_count; i++) {
    VkImageView attachments[] = {
      swapchain->image_views[i]
    };
    VkFramebufferCreateInfo framebuffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = swapchain->render_pass,
      .attachmentCount = 1,
      .pAttachments = attachments,
      .width = swapchain->extent.width,
      .height = swapchain->extent.height,
      .layers = 1,
    };
    ASO_VK_CHECK(vkCreateFramebuffer(device->device, &framebuffer_info, NULL, &swapchain->framebuffers[i]), "Failed to create framebuffer");
  }
}

// REGION: RENDER PASS

void aso_vk_swapchain_create_render_pass(aso_vk_swapchain *swapchain, const aso_vk_device *device) {
  ASSERT(swapchain != NULL);
  ASSERT(device != NULL);

  VkAttachmentDescription color_attachment = {
    .format = swapchain->format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  // subpass
  VkAttachmentReference color_attachment_reference = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_reference,
  };

  VkRenderPassCreateInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color_attachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
  };

  // create a dependency in order to wait until we have an image from the swap chain
  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  ASO_VK_CHECK(vkCreateRenderPass(device->device, &render_pass_info, NULL, &swapchain->render_pass), "Failed to create render pass");
}

// REGION: CLEANUP

void aso_vk_swapchain_cleanup(aso_vk_swapchain *swapchain, const aso_vk_device *device) {
  ASSERT(device->device != NULL);
  ASSERT(swapchain != NULL);

  // framebuffers & image views
  for (size_t i = 0; i < swapchain->image_count; i++) {
    vkDestroyFramebuffer(device->device, swapchain->framebuffers[i], NULL);
    vkDestroyImageView(device->device, swapchain->image_views[i], NULL);
  }

  vkDestroySwapchainKHR(device->device, swapchain->handle, NULL); 
}
