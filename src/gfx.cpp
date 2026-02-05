#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "core.h"
#include "gfx.h"
#include "mem.h"
#include "window.h"

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
      layer_found = true;
      break;
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

  g_ctx->scratch->offset = scratch_save;
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

    VkExtent2D extent = {
      CLAMP(width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width),
      CLAMP(height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height)
    };

    return extent;
  }
}

// REGION: CLEANUP

void aso_cleanup_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  vkDeviceWaitIdle(vulkan_ctx->device);
  vkDestroySwapchainKHR(vulkan_ctx->device, vulkan_ctx->swap_chain, nullptr);
  vkDestroyDevice(vulkan_ctx->device, nullptr);
  vkDestroySurfaceKHR(vulkan_ctx->instance, vulkan_ctx->surface, nullptr);
  vkDestroyInstance(vulkan_ctx->instance, nullptr);
  aso_arena_destroy(vulkan_ctx->arena);
  // NOTE: physical_device and queues are cleaned up implicitly
}
