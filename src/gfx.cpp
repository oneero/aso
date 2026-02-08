#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "core.h"
#include "gfx.h"
#include "mem.h"
#include "window.h"
#include "io.h"

// TODO: refactor scratch arena usage
// TODO: cleanup
// TODO: replace exit()s

const char* aso_vulkan_validation_layers[ASO_VULKAN_VALIDATION_LAYER_COUNT] = {
  "VK_LAYER_KHRONOS_validation"
};

const char* aso_vulkan_device_extensions[ASO_VULKAN_DEVICE_EXTENSION_COUNT] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  vulkan_ctx->arena = aso_arena_create();

  aso_create_vulkan_instance(vulkan_ctx);
  if (!aso_create_vulkan_surface(g_ctx->window.handle, vulkan_ctx)) {
    aso_log("Failed to create SDL3 Vulkan surface\n");
    exit(1);
  }
  aso_select_physical_device(vulkan_ctx);
  aso_create_vulkan_logical_device(vulkan_ctx);
  aso_create_swap_chain(vulkan_ctx);
  aso_create_image_views(vulkan_ctx);
  aso_create_render_pass(vulkan_ctx);
  aso_create_graphics_pipeline(vulkan_ctx);
  aso_create_framebuffers(vulkan_ctx);
  aso_create_command_pool(vulkan_ctx);
  aso_create_command_buffers(vulkan_ctx);
  aso_create_sync_objects(vulkan_ctx);

  aso_arena_free(vulkan_ctx->arena);
}

// REGION: INSTANCE

void aso_create_vulkan_instance(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "aso";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName = "aso";
  app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instance_create_info = {};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;

  // REGION: LAYERS

  // list avaiable layers
  u32 available_layer_count = 0;
  VkLayerProperties *layers = aso_get_available_vulkan_layers(vulkan_ctx->arena, &available_layer_count);
  aso_log("Available Vulkan layers:\n");
  for (int i = 0; i < available_layer_count; i++) {
    aso_log(" %s\n", layers[i].layerName);
  }

  if (aso_enable_vulkan_validation_layers && !aso_check_vulkan_validation_layer_support(layers, available_layer_count)) {
    aso_log("Vulkan validation layers not supported\n");
    aso_enable_vulkan_validation_layers = false;
  }

  instance_create_info.ppEnabledLayerNames = aso_get_vulkan_layers(&instance_create_info.enabledLayerCount);

  aso_log("Enabled Vulkan layers:\n");
  for (int i = 0; i < instance_create_info.enabledLayerCount; i++) {
    aso_log(" %s\n", instance_create_info.ppEnabledLayerNames[i]);
  }

  // REGION: EXTENSIONS

  // list available extensions
  u32 available_extension_count = 0;
  VkExtensionProperties *extensions = aso_get_available_vulkan_extensions(vulkan_ctx->arena, &available_extension_count); 
  aso_log("Available Vulkan extensions:\n");
  for (u32 i = 0; i < available_extension_count; i++) {
    aso_log(" %s\n", extensions[i].extensionName);
  }

  // configure extensions we want enabled
  instance_create_info.ppEnabledExtensionNames = aso_get_vulkan_extensions(&instance_create_info.enabledExtensionCount);

  aso_log("Enabled Vulkan extensions:\n");
  for (int i = 0; i < instance_create_info.enabledExtensionCount; i++) {
    aso_log(" %s\n", instance_create_info.ppEnabledExtensionNames[i]);
  }

  // REGION: INSTANCE
  // TODO: setup debug message callback
  // TODO: use an explicit allocator
  VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &vulkan_ctx->instance), "Failed to create Vulkan instance\n");
}

VkExtensionProperties* aso_get_available_vulkan_extensions(aso_arena *arena, u32 *count) {
  assert(arena != NULL);
  assert(count != NULL);

  VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, count, NULL), "Failed to get available Vulkan extension count\n");

  VkExtensionProperties *extensions = ASO_ARENA_ALLOC_ARRAY(arena, VkExtensionProperties, *count);

  VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, count, extensions), "Failed to enumerate available Vulkan extensions\n");
  return extensions;
}

// currently only adds extensions required by window
char const * const * aso_get_vulkan_extensions(u32 *count) {
  assert(count != NULL);

  // get extensions from window framework (currently just SDL)
  char const * const * window_extensions = aso_get_window_vulkan_extensions(count);
  if (window_extensions == NULL) {
    aso_log("Failed to fetch window Vulkan extensions\n");
    exit(1);
  }

  if (!aso_enable_vulkan_validation_layers) {
    return window_extensions;
  }

  // TODO: insert debug utils extension when using validation layers
  return window_extensions;
}

VkLayerProperties* aso_get_available_vulkan_layers(aso_arena *arena, u32 *count) {
  assert(arena != NULL);
  assert(count != NULL);

  VK_CHECK(vkEnumerateInstanceLayerProperties(count, NULL), "Failed to get available Vulkan layer count\n");

  VkLayerProperties *layers = ASO_ARENA_ALLOC_ARRAY(arena, VkLayerProperties, *count);

  VK_CHECK(vkEnumerateInstanceLayerProperties(count, layers), "Failed to enumerate available Vulkan layers\n");

  return layers;
}

bool aso_check_vulkan_validation_layer_support(VkLayerProperties *available_layers, u32 count) {
  for (int i = 0; i < ASO_VULKAN_VALIDATION_LAYER_COUNT; i++) {
    bool layer_found = false;

    for (int j = 0; j < count; j++) {
      if (strcmp(aso_vulkan_validation_layers[i], available_layers[j].layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
}

// currently only adds validation layers
char const * const * aso_get_vulkan_layers(u32 *count) {
  assert(count != NULL);

  *count = 0;
  if (aso_enable_vulkan_validation_layers) {
    *count += ASO_VULKAN_VALIDATION_LAYER_COUNT;
    return aso_vulkan_validation_layers;
  }
  return NULL;
}

// REGION: PHYSICAL DEVICE

void aso_select_physical_device(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  vulkan_ctx->physical_device = VK_NULL_HANDLE;
 
  aso_log("Finding suitable GPU..\n");
  
  u32 device_count = 0;
  vkEnumeratePhysicalDevices(vulkan_ctx->instance, &device_count, nullptr);
  if (device_count == 0) {
    aso_log("Failed to find any devices supporting Vulkan\n");
    exit(1);
  }

  VkPhysicalDevice *devices = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkPhysicalDevice, device_count);

  VK_CHECK(vkEnumeratePhysicalDevices(vulkan_ctx->instance, &device_count, devices), "Failed to enumerate GPUs");

  for (int i = 0; i < device_count; i++) {
    if (aso_is_device_suitable(vulkan_ctx, devices[i])) {
      vulkan_ctx->physical_device = devices[i];
      aso_log(" = suitable\n");
      break;
    }
  }

  if (vulkan_ctx->physical_device == VK_NULL_HANDLE) {
    aso_log("Failed to find suitable GPU\n");
  }
}

bool aso_is_device_suitable(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device) {
  assert(vulkan_ctx != NULL);
  assert(physical_device != NULL);

  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  aso_log("\n%s\n", device_properties.deviceName);

  aso_vulkan_queue_family_indices indices = aso_get_vulkan_family_indices(vulkan_ctx, physical_device);
  bool extensions_supported = aso_check_device_extension_support(vulkan_ctx->arena, physical_device);

  // only check for swap chain support if the extension is supported
  bool swap_chain_ok = false;
  if (extensions_supported) {
    // TODO: cache the details for later
    aso_vulkan_swap_chain_support_details details = aso_query_swap_chain_support(vulkan_ctx, physical_device);
    swap_chain_ok = details.formats_count > 0 && details.present_modes_count > 0;
  }

  return indices.has_graphics_family && indices.has_present_family && extensions_supported && swap_chain_ok;
}

aso_vulkan_queue_family_indices aso_get_vulkan_family_indices(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device) {
  assert(vulkan_ctx != NULL);
  aso_vulkan_queue_family_indices indices = {};

  u32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
  VkQueueFamilyProperties *queue_families = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkQueueFamilyProperties, queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

  for (int i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
      indices.has_graphics_family = true;
    }

    VkBool32 presentation_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, vulkan_ctx->surface, &presentation_support);
    if (presentation_support) {
      indices.present_family = i;
      indices.has_present_family = true;
    }

    if (indices.has_graphics_family && indices.has_present_family) {
      break;
    }
  }

  return indices;
}

bool aso_check_device_extension_support(aso_arena *arena, VkPhysicalDevice physical_device) {
  assert(physical_device != NULL);

  u32 extension_count;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

  VkExtensionProperties *available_extensions = ASO_ARENA_ALLOC_ARRAY(arena, VkExtensionProperties, extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions);

  // check each required extension is supported
  for (int i = 0; i < ASO_VULKAN_DEVICE_EXTENSION_COUNT; i++) {
    bool found = false;
    for (int j = 0; j < extension_count; j++) {
      if (strcmp(aso_vulkan_device_extensions[i], available_extensions[j].extensionName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      aso_log(" Missing support for %s\n", aso_vulkan_device_extensions[i]);
      return false;
    }
  }

  return true;
}

aso_vulkan_swap_chain_support_details aso_query_swap_chain_support(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device) {
  assert(vulkan_ctx != NULL);
  assert(physical_device != NULL);

  aso_vulkan_swap_chain_support_details details = {0};

  // capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, vulkan_ctx->surface, &details.capabilities);

  // formats
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vulkan_ctx->surface, &details.formats_count, NULL);
  details.formats = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkSurfaceFormatKHR, details.formats_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vulkan_ctx->surface, &details.formats_count, details.formats);

  // present modes
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vulkan_ctx->surface, &details.present_modes_count, NULL);
  details.present_modes = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkPresentModeKHR, details.present_modes_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vulkan_ctx->surface, &details.present_modes_count, details.present_modes);

  return details;
}


// REGION: LOGICAL DEVICE

void aso_create_vulkan_logical_device(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  // TODO: cache this earlier?
  aso_vulkan_queue_family_indices indices = aso_get_vulkan_family_indices(vulkan_ctx, vulkan_ctx->physical_device);

  // use a bitmap to produce an array of unique VkDeviceQueueCreateInfo
  // NOTE: max 8 unique queue families
  u8 unique_families = 0;
  u8 unique_family_count = 0;
  unique_families |= (1 << indices.graphics_family);
  unique_families |= (1 << indices.present_family);

  VkDeviceQueueCreateInfo queue_create_infos[8];
  float queue_priority = 1.0f;

  for (u8 i = 0; i < 8; i++) {
    if (unique_families & (1 << i)) {
      queue_create_infos[unique_family_count++] = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = i,
        .queueCount = 1,
        .pQueuePriorities = & queue_priority,
      };   
    }
  }

  VkPhysicalDeviceFeatures device_features = {}; // TODO: get back to this

  VkDeviceCreateInfo device_create_info = {};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = queue_create_infos;
  device_create_info.queueCreateInfoCount = unique_family_count;
  device_create_info.pEnabledFeatures = &device_features;

  device_create_info.enabledExtensionCount = ASO_VULKAN_DEVICE_EXTENSION_COUNT;
  device_create_info.ppEnabledExtensionNames = aso_vulkan_device_extensions;

  // validation layers are already defined at instance level, so there are
  // not necessary for up-to-date Vulkan SDK, but included for combatibility
  if (aso_enable_vulkan_validation_layers) {
    device_create_info.enabledLayerCount = ASO_VULKAN_VALIDATION_LAYER_COUNT;
    device_create_info.ppEnabledLayerNames = aso_vulkan_validation_layers; 
  } else {
    device_create_info.enabledLayerCount = 0;
  }

  VK_CHECK(vkCreateDevice(vulkan_ctx->physical_device, &device_create_info, nullptr, &vulkan_ctx->device), "Failed to create logical device\n");

  vkGetDeviceQueue(vulkan_ctx->device, indices.graphics_family, 0, &vulkan_ctx->graphics_queue);
  vkGetDeviceQueue(vulkan_ctx->device, indices.present_family, 0, &vulkan_ctx->presentation_queue);
}

// REGION: SWAP CHAIN

void aso_create_swap_chain(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  aso_log("\nCreating swap chain..\n");

  size_t scratch_save = g_ctx->scratch->offset;

  aso_vulkan_swap_chain_support_details details = aso_query_swap_chain_support(vulkan_ctx, vulkan_ctx->physical_device);

  // surface format
  VkSurfaceFormatKHR surface_format = details.formats[0];
  for (int i = 0; i < details.formats_count; i++) {
    if (details.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surface_format = details.formats[i];
      break;
    }
  }

  // present mode
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (int i = 0; i < details.present_modes_count; i++) {
    if (details.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }
  
  // extent
  VkExtent2D extent = aso_select_swap_extent(&details.capabilities);
  aso_log(" Extent: %d x %d\n", extent.width, extent.height);

  u32 image_count = details.capabilities.minImageCount + 1;
  if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount) {
    image_count = details.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = vulkan_ctx->surface;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.minImageCount = image_count;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  aso_vulkan_queue_family_indices indices = aso_get_vulkan_family_indices(vulkan_ctx, vulkan_ctx->physical_device);
  u32 queue_family_indices[] = { indices.graphics_family, indices.present_family };

  // if the queue families are different, we use concurrent mode to share images
  // NOTE: exclusive mode is ideal but requires explicit ownership transfers
  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2; // TODO: this should not be hardcoded
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = details.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  VK_CHECK(vkCreateSwapchainKHR(vulkan_ctx->device, &create_info, nullptr, &vulkan_ctx->swap_chain), "Failed to create swap chain\n");

  // retrieve swap chain image handles
  vkGetSwapchainImagesKHR(vulkan_ctx->device, vulkan_ctx->swap_chain, &vulkan_ctx->swap_chain_images_count, nullptr);
  vulkan_ctx->swap_chain_images = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkImage, vulkan_ctx->swap_chain_images_count);
  vkGetSwapchainImagesKHR(vulkan_ctx->device, vulkan_ctx->swap_chain, &vulkan_ctx->swap_chain_images_count, vulkan_ctx->swap_chain_images);

  // store format and extent for later use
  vulkan_ctx->swap_chain_format = surface_format.format;
  vulkan_ctx->swap_chain_extent = extent;
}

VkExtent2D aso_select_swap_extent(VkSurfaceCapabilitiesKHR *capabilities) {
  assert(capabilities != NULL);

  // check current extents for special value
  // -> go with already set extent
  // -> special value set: set swap extent here and the surface will conform
  if (capabilities->currentExtent.width != 0xFFFFFFFF) {
    return capabilities->currentExtent;
  } else {
    int width, height;
    aso_get_window_size(&g_ctx->window, &width, &height);
    aso_log("Window extent: %dx%d\n", width, height);
    VkExtent2D extent = {
      CLAMP(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width),
      CLAMP(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height)
    };

    return extent;
  }
}

void aso_recreate_swap_chain(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  // check if the window is minimized
  int width, heigth;
  aso_get_window_size(&g_ctx->window, &width, &heigth);
  while (width == 0 || heigth == 0) {
    aso_wait_for_sdl_event(NULL);
    aso_get_window_size(&g_ctx->window, &width, &heigth);
  }

  vkDeviceWaitIdle(vulkan_ctx->device);

  aso_cleanup_swap_chain(vulkan_ctx);

  aso_create_swap_chain(vulkan_ctx);
  aso_create_image_views(vulkan_ctx);
  aso_create_framebuffers(vulkan_ctx);
}

void aso_cleanup_swap_chain(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  // framebuffers
  for (size_t i = 0; i < vulkan_ctx->swap_chain_framebuffers_count; i++) {
    vkDestroyFramebuffer(vulkan_ctx->device, vulkan_ctx->swap_chain_framebuffers[i], nullptr);
  }

  // swap chain image views
  for (size_t i = 0; i < vulkan_ctx->swap_chain_image_views_count; i++) {
    vkDestroyImageView(vulkan_ctx->device, vulkan_ctx->swap_chain_image_views[i], nullptr);
  }

  vkDestroySwapchainKHR(vulkan_ctx->device, vulkan_ctx->swap_chain, nullptr);
}

// REGION: IMAGE VIEWS

void aso_create_image_views(aso_vulkan_ctx *vulkan_ctx) {
  vulkan_ctx->swap_chain_image_views_count = vulkan_ctx->swap_chain_images_count;
  vulkan_ctx->swap_chain_image_views = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkImageView, vulkan_ctx->swap_chain_image_views_count);
  for (size_t i = 0; i < vulkan_ctx->swap_chain_images_count; i++) {
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = vulkan_ctx->swap_chain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = vulkan_ctx->swap_chain_format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // subresourceRange describes image view usage details
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(vulkan_ctx->device, &create_info, nullptr, &vulkan_ctx->swap_chain_image_views[i]), "Failed to create image view");
  }
}

// REGION: GRAPHICS PIPELINE

void aso_create_graphics_pipeline(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);
  assert(vulkan_ctx->arena != NULL);

  // load shader bytecode into shader modules

  size_t check_point = vulkan_ctx->arena->offset;

  long vert_shader_code_size = 0;
  long frag_shader_code_size = 0;
  u8 *vert_shader_code = aso_read_binary_file(vulkan_ctx->arena, "assets/vert.spv", &vert_shader_code_size);
  u8 *frag_shader_code = aso_read_binary_file(vulkan_ctx->arena, "assets/frag.spv", &frag_shader_code_size);
  VkShaderModule vert_shader_module = aso_create_shader_module(vulkan_ctx->device, vert_shader_code, vert_shader_code_size);
  VkShaderModule frag_shader_module = aso_create_shader_module(vulkan_ctx->device, frag_shader_code, frag_shader_code_size);

  // we dont need the shader code anymore
  vulkan_ctx->arena->offset = check_point;

  // shader stages
  
  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName = "main";
  vert_shader_stage_info.pSpecializationInfo = nullptr; // no constants

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName = "main";
  frag_shader_stage_info.pSpecializationInfo = nullptr; // no constants

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  // vertex input

  // vertex info is hardcoded in shader for now
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions = nullptr; // optional
  vertex_input_info.vertexAttributeDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions = nullptr; // optional

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  // viewport and scissor as dynamic states to be specified at draw time
  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state = {};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates = dynamic_states;

  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  // rasterizer

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // optional
  rasterizer.depthBiasClamp = 0.0f; // optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // optional

  // TODO: enable later
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // optional
  multisampling.pSampleMask = nullptr; // optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // optional
  multisampling.alphaToOneEnable = VK_FALSE; // optional

  // TODO: add depth and stencil states?

  // color blending
  // NOTE:: alpha blending disabled for now

  VkPipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // optional
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // optional
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // optional
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // optional

  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY; // optional
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f; // optional
  color_blending.blendConstants[1] = 0.0f; // optional
  color_blending.blendConstants[2] = 0.0f; // optional
  color_blending.blendConstants[3] = 0.0f; // optional
  
  // pipeline layout for uniforms

  VkPipelineLayoutCreateInfo pipeline_layout_info = {};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 0; // optional
  pipeline_layout_info.pSetLayouts = nullptr; // optional
  pipeline_layout_info.pushConstantRangeCount = 0; // optional
  pipeline_layout_info.pPushConstantRanges = nullptr; // optional

  VK_CHECK(vkCreatePipelineLayout(vulkan_ctx->device, &pipeline_layout_info, nullptr, &vulkan_ctx->pipeline_layout), "Failed to create pipeline layout\n");

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;

  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = nullptr; // optional
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;

  pipeline_info.layout = vulkan_ctx->pipeline_layout;
  pipeline_info.renderPass = vulkan_ctx->render_pass;
  pipeline_info.subpass = 0;

  // if using derivative pipeline..
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // optional
  pipeline_info.basePipelineIndex = -1; // optional
 
  VK_CHECK(vkCreateGraphicsPipelines(vulkan_ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &vulkan_ctx->graphics_pipeline), "Failed to create graphics pipeline\n");

  vkDestroyShaderModule(vulkan_ctx->device, frag_shader_module, nullptr);
  vkDestroyShaderModule(vulkan_ctx->device, vert_shader_module, nullptr);
}

VkShaderModule aso_create_shader_module(VkDevice device, u8 *shader_code, long code_size) {
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code_size;
  create_info.pCode = (const u32 *)shader_code;
  VkShaderModule module;
  VK_CHECK(vkCreateShaderModule(device, &create_info, nullptr, &module), "Failed to create shader module\n");
  return module;
}

// REGION: RENDER PASS

void aso_create_render_pass(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  VkAttachmentDescription color_attachment = {};
  color_attachment.format = vulkan_ctx->swap_chain_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // subpass
  VkAttachmentReference color_attachment_reference = {};
  color_attachment_reference.attachment = 0;
  color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_reference;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;

  // create a dependency in order to wait until we have an image from the swap chain
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  VK_CHECK(vkCreateRenderPass(vulkan_ctx->device, &render_pass_info, nullptr, &vulkan_ctx->render_pass), "Failed to create render pass\n");
}

// REGION: FRAMEBUFFERS

void aso_create_framebuffers(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  // we create a framebuffer for each imageview in the swap chain

  vulkan_ctx->swap_chain_framebuffers_count = vulkan_ctx->swap_chain_image_views_count;
  vulkan_ctx->swap_chain_framebuffers = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkFramebuffer, vulkan_ctx->swap_chain_framebuffers_count);

  for (size_t i = 0; i < vulkan_ctx->swap_chain_image_views_count; i++) {
    VkImageView attachments[] = {
      vulkan_ctx->swap_chain_image_views[i]
    };
    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = vulkan_ctx->render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = vulkan_ctx->swap_chain_extent.width;
    framebuffer_info.height = vulkan_ctx->swap_chain_extent.height;
    framebuffer_info.layers = 1;

    VK_CHECK(vkCreateFramebuffer(vulkan_ctx->device, &framebuffer_info, nullptr, &vulkan_ctx->swap_chain_framebuffers[i]), "Failed to create framebuffer\n");
  }
}

// REGION: COMMAND POOL

void aso_create_command_pool(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  aso_vulkan_queue_family_indices queue_family_indices = aso_get_vulkan_family_indices(vulkan_ctx, vulkan_ctx->physical_device);

  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // reset command buffers every frame
  pool_info.queueFamilyIndex = queue_family_indices.graphics_family;

  VK_CHECK(vkCreateCommandPool(vulkan_ctx->device, &pool_info, nullptr, &vulkan_ctx->command_pool), "Failed to create command pool\n");
}

// REGION: COMMAND BUFFER

void aso_create_command_buffers(aso_vulkan_ctx *vulkan_ctx) {
  assert (vulkan_ctx != NULL);

  vulkan_ctx->command_buffers = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkCommandBuffer, ASO_VULKAN_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = vulkan_ctx->command_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = ASO_VULKAN_FRAMES_IN_FLIGHT;

  VK_CHECK(vkAllocateCommandBuffers(vulkan_ctx->device, &alloc_info, vulkan_ctx->command_buffers), "Failed to allocate command buffers\n");
}

void aso_record_command_buffer(aso_vulkan_ctx *vulkan_ctx, u32 image_index) {
  assert(vulkan_ctx != NULL);

  u32 f = vulkan_ctx->frame;

  // begin

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = 0; // optional
  begin_info.pInheritanceInfo = nullptr; // optional

  VK_CHECK(vkBeginCommandBuffer(vulkan_ctx->command_buffers[f], &begin_info), "Failed to begin recording command buffer\n");

  // start render pass

  VkRenderPassBeginInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = vulkan_ctx->render_pass;
  render_pass_info.framebuffer = vulkan_ctx->swap_chain_framebuffers[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = vulkan_ctx->swap_chain_extent;
  
  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;

  vkCmdBeginRenderPass(vulkan_ctx->command_buffers[f], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  // bind pipeline

  vkCmdBindPipeline(vulkan_ctx->command_buffers[f], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_ctx->graphics_pipeline);

  // define viewport and scissor as they were set to dynamic

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) vulkan_ctx->swap_chain_extent.width;
  viewport.height = (float) vulkan_ctx->swap_chain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(vulkan_ctx->command_buffers[f], 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = vulkan_ctx->swap_chain_extent;
  vkCmdSetScissor(vulkan_ctx->command_buffers[f], 0, 1, &scissor);

  // draw!

  vkCmdDraw(vulkan_ctx->command_buffers[f], 3, 1, 0, 0);

  vkCmdEndRenderPass(vulkan_ctx->command_buffers[f]);

  VK_CHECK(vkEndCommandBuffer(vulkan_ctx->command_buffers[f]), "Failed to record command buffer\n");
}

void aso_create_sync_objects(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  vulkan_ctx->image_available_semaphores = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkSemaphore, ASO_VULKAN_FRAMES_IN_FLIGHT);
  vulkan_ctx->render_finished_semaphores = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkSemaphore, ASO_VULKAN_FRAMES_IN_FLIGHT);
  vulkan_ctx->in_flight_fences = ASO_ARENA_ALLOC_ARRAY(vulkan_ctx->arena, VkFence, ASO_VULKAN_FRAMES_IN_FLIGHT);


  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so we dont block forever on first frame

  for (size_t i = 0; i < ASO_VULKAN_FRAMES_IN_FLIGHT; i++) {
    VK_CHECK(vkCreateSemaphore(vulkan_ctx->device, &semaphore_info, nullptr, &vulkan_ctx->image_available_semaphores[i]), "Failed to create semaphore\n");
    VK_CHECK(vkCreateSemaphore(vulkan_ctx->device, &semaphore_info, nullptr, &vulkan_ctx->render_finished_semaphores[i]), "Failed to create semaphore\n");
    VK_CHECK(vkCreateFence(vulkan_ctx->device, &fence_info, nullptr, &vulkan_ctx->in_flight_fences[i]), "Failed to create fence\n");
  }
}

// REGION: DRAW FRAME

void aso_draw_frame(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  u32 f = vulkan_ctx->frame;
  aso_log("drawing frame %d\n", f);

  aso_log("waiting for fence..\n");

  vkWaitForFences(vulkan_ctx->device, 1, &vulkan_ctx->in_flight_fences[f], VK_TRUE, UINT64_MAX);
  aso_log("acquiring image\n");

  u32 image_index;
  VkResult image_acquire_result = vkAcquireNextImageKHR(vulkan_ctx->device, vulkan_ctx->swap_chain, UINT64_MAX, vulkan_ctx->image_available_semaphores[f], VK_NULL_HANDLE, &image_index);
  if (image_acquire_result == VK_ERROR_OUT_OF_DATE_KHR ||
      image_acquire_result == VK_SUBOPTIMAL_KHR ||
      vulkan_ctx->window_resized) {
    aso_log("Swap chain image out of date\n");
    vulkan_ctx->window_resized = false;
    aso_recreate_swap_chain(vulkan_ctx);
    return;
  } else if (image_acquire_result != VK_SUCCESS) {
    aso_log("Failed to acquire swap chain image\n");
  }

  vkResetFences(vulkan_ctx->device, 1, &vulkan_ctx->in_flight_fences[f]);
  
  aso_log("resetting cmd buffer\n");
  vkResetCommandBuffer(vulkan_ctx->command_buffers[f], 0);
  aso_log("recording cmd buffer\n");
  aso_record_command_buffer(vulkan_ctx, image_index);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {vulkan_ctx->image_available_semaphores[f]};
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &vulkan_ctx->command_buffers[f];

  VkSemaphore signal_semaphores[] = {vulkan_ctx->render_finished_semaphores[f]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;
  aso_log("submitting to graphics queue\n");

  VK_CHECK(vkQueueSubmit(vulkan_ctx->graphics_queue, 1, &submit_info, vulkan_ctx->in_flight_fences[f]), "Failed to submit draw command buffer\n");

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swap_chains[] = {vulkan_ctx->swap_chain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &image_index;

  present_info.pResults = nullptr; // optional
  aso_log("submitting to present queue\n");

  VkResult present_result = vkQueuePresentKHR(vulkan_ctx->presentation_queue, &present_info);
  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR ||
      vulkan_ctx->window_resized) {
    aso_log("Swap chain image out of date\n");
    vulkan_ctx->window_resized = false;
    aso_recreate_swap_chain(vulkan_ctx);
  } else if (present_result != VK_SUCCESS) {
    aso_log("Failed to acquire swap chain image\n");
  }
  aso_log("done\n\n");

  vulkan_ctx->frame = (f + 1) % ASO_VULKAN_FRAMES_IN_FLIGHT;
}

// REGION: CLEANUP

void aso_cleanup_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  vkDeviceWaitIdle(vulkan_ctx->device);

  aso_cleanup_swap_chain(vulkan_ctx);

  vkDestroyPipeline(vulkan_ctx->device, vulkan_ctx->graphics_pipeline, nullptr);
  vkDestroyPipelineLayout(vulkan_ctx->device, vulkan_ctx->pipeline_layout, nullptr);
  vkDestroyRenderPass(vulkan_ctx->device, vulkan_ctx->render_pass, nullptr);
  
  for (size_t i = 0; i < ASO_VULKAN_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(vulkan_ctx->device, vulkan_ctx->image_available_semaphores[i], nullptr);
    vkDestroySemaphore(vulkan_ctx->device, vulkan_ctx->render_finished_semaphores[i], nullptr);
    vkDestroyFence(vulkan_ctx->device, vulkan_ctx->in_flight_fences[i], nullptr);
  }

  // NOTE: Destroying command bool will implicitly free command buffers
  vkDestroyCommandPool(vulkan_ctx->device, vulkan_ctx->command_pool, nullptr);

  vkDestroyDevice(vulkan_ctx->device, nullptr);
  vkDestroySurfaceKHR(vulkan_ctx->instance, vulkan_ctx->surface, nullptr);
  vkDestroyInstance(vulkan_ctx->instance, nullptr);
  aso_arena_destroy(vulkan_ctx->arena);
  // NOTE: physical_device and queues are cleaned up implicitly
}
