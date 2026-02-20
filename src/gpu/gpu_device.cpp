#include <cstddef>
#include <cstring>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "gpu.h"
#include "window.h"
#include "gpu/gpu_device.h"

const char* aso_vulkan_validation_layers[ASO_VK_VALIDATION_LAYER_COUNT] = {
  "VK_LAYER_KHRONOS_validation"
};

const char* aso_vulkan_device_extensions[ASO_VK_DEVICE_EXTENSION_COUNT] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void aso_vk_device_init(aso_arena *scratch, aso_vk_device* device) {
  ASSERT(scratch != NULL);
  ASSERT(device != NULL);

  aso_vk_instance_init(scratch, device);

  bool surface_ok = aso_create_vulkan_surface(g_ctx->window.handle, device->instance, &device->surface);
  ASSERT(surface_ok && "Failed to create SDL3 surface for Vulkan");

  aso_vk_select_physical_device(scratch, device);
  device->queue_families = aso_vk_get_queue_families(scratch, device->physical_device, device->surface);
  aso_vk_create_logical_device(device);

  vkGetPhysicalDeviceMemoryProperties(device->physical_device, &device->memory_properties);
}

// REGION: INSTANCE

void aso_vk_instance_init(aso_arena *scratch, aso_vk_device *device) {
  ASSERT(scratch != NULL);
  ASSERT(device != NULL);

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "aso",
    .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
    .pEngineName = "aso",
    .engineVersion = VK_MAKE_VERSION(0, 1, 0),
    .apiVersion = VK_API_VERSION_1_0,
  };

  VkInstanceCreateInfo instance_create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,

    .ppEnabledLayerNames = aso_vk_resolve_layers(scratch, &instance_create_info.enabledLayerCount, &device->use_validation),
    .ppEnabledExtensionNames = aso_vk_resolve_extensions(scratch, &instance_create_info.enabledExtensionCount, device->use_validation),
  };

  ASO_VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &device->instance), "Failed to create Vulkan instance");
  
  // TODO: setup debug message callback
}

// REGION: LAYERS

// queries avalable layers
// checks for validation support
// defines to-be-enabled layers
char const * const * aso_vk_resolve_layers(aso_arena *scratch, u32 *count, bool *use_validation) {
  ASSERT(count != NULL);

  // list available layers
  u32 available_layer_count = 0;
  VkLayerProperties *available_layers = aso_vk_get_available_layers(scratch, &available_layer_count);

#ifdef BUILD_DEBUG
  D_LOG("Available Vulkan layers:");
  for (u32 i = 0; i < available_layer_count; i++) {
    D_LOG(" %s", available_layers[i].layerName);
  }
#endif // BUILD_DEBUG

#ifdef ASO_VK_VALIDATION_LAYERS
  *use_validation = aso_vk_check_validation_layer_support(available_layers, available_layer_count);
  D_LOG("Vulkan validation layers enabled: %b", *use_validation);
#endif // ASO_VK_VALIDATION_LAYERS
  
  *count = 0;
  char const* const* enabled_layers = NULL;
  if (use_validation) {
    *count += ASO_VK_VALIDATION_LAYER_COUNT;
    enabled_layers = aso_vulkan_validation_layers;
  }

#ifdef BUILD_DEBUG
  D_LOG("Enabled Vulkan layers:");
  for (u32 i = 0; i < *count; i++) {
    D_LOG(" %s", enabled_layers[i]);
  }
#endif // BUILD_DEBUG

  return enabled_layers;
}

VkLayerProperties* aso_vk_get_available_layers(aso_arena *scratch, u32 *count) {
  ASSERT(scratch != NULL);
  ASSERT(count != NULL);
  ASO_VK_CHECK(vkEnumerateInstanceLayerProperties(count, NULL), "Failed to get available Vulkan layer count");
  VkLayerProperties *layers = ASO_ARENA_ALLOC_ARRAY(scratch, VkLayerProperties, *count);
  ASO_VK_CHECK(vkEnumerateInstanceLayerProperties(count, layers), "Failed to enumerate available Vulkan layers");
  return layers;
}

bool aso_vk_check_validation_layer_support(VkLayerProperties *available_layers, u32 count) {
  for (int i = 0; i < ASO_VK_VALIDATION_LAYER_COUNT; i++) {
    bool layer_found = false;

    for (u32 j = 0; j < count; j++) {
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

// REGION: EXTENSIONS

// queries available extensions
// defines to-be-enabled extensions
char const * const * aso_vk_resolve_extensions(aso_arena *scratch, u32 *count, bool use_validation) {
  ASSERT(scratch != NULL);
  ASSERT(count != NULL);

  // list available extensions
  u32 available_extension_count = 0;
  VkExtensionProperties *extensions = aso_vk_get_available_extensions(scratch, &available_extension_count); 
#ifdef BUILD_DEBUG 
  D_LOG("Available Vulkan extensions:");
  for (u32 i = 0; i < available_extension_count; i++) {
    D_LOG(" %s", extensions[i].extensionName);
  }
#endif // BUILD_DEBUG  

  // get extensions from window framework
  char const * const * window_extensions = aso_get_window_vulkan_extensions(count);
  ASSERT(window_extensions != NULL && "Failed to fetch window Vulkan extensions");

  char const * const * enabled_extensions = window_extensions;
  if (use_validation) {
    // TODO: insert debug utils extension when using validation layers
  }
#ifdef BUILD_DEBUG
  D_LOG("Enabled Vulkan extensions:");
  for (u32 i = 0; i < *count; i++) {
    D_LOG(" %s", enabled_extensions[i]);
  }
#endif // BUILD_DEBUG

  return enabled_extensions;
}

VkExtensionProperties* aso_vk_get_available_extensions(aso_arena *scratch, u32 *count) {
  ASSERT(scratch != NULL);
  ASSERT(count != NULL);
  ASO_VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, count, NULL), "Failed to get available Vulkan extension count");
  VkExtensionProperties *extensions = ASO_ARENA_ALLOC_ARRAY(scratch, VkExtensionProperties, *count);
  ASO_VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, count, extensions), "Failed to enumerate available Vulkan extensions");
  return extensions;
}

bool aso_vk_check_device_extension_support(aso_arena *scratch, VkPhysicalDevice physical_device) {
  ASSERT(scratch != NULL);
  ASSERT(physical_device != NULL);

  u32 extension_count;
  vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);

  VkExtensionProperties *available_extensions = ASO_ARENA_ALLOC_ARRAY(scratch, VkExtensionProperties, extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, available_extensions);

  // check each required extension is supported
  for (int i = 0; i < ASO_VK_DEVICE_EXTENSION_COUNT; i++) {
    bool found = false;
    for (u32 j = 0; j < extension_count; j++) {
      if (strcmp(aso_vulkan_device_extensions[i], available_extensions[j].extensionName) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      LOG(" Missing support for %s", aso_vulkan_device_extensions[i]);
      return false;
    }
  }

  return true;
}

// REGION: PHYSICAL DEVICE

void aso_vk_select_physical_device(aso_arena *scratch, aso_vk_device *device) {
  ASSERT(scratch != NULL);
  ASSERT(device != NULL);

  device->physical_device = VK_NULL_HANDLE;
 
  D_LOG("Finding suitable GPU..");
  
  u32 device_count = 0;
  vkEnumeratePhysicalDevices(device->instance, &device_count, NULL);
  ASSERT(device_count != 0 && "Failed to find any devices supporting Vulkan");

  VkPhysicalDevice *devices = ASO_ARENA_ALLOC_ARRAY(scratch, VkPhysicalDevice, device_count);

  ASO_VK_CHECK(vkEnumeratePhysicalDevices(device->instance, &device_count, devices), "Failed to enumerate GPUs");

  for (u32 i = 0; i < device_count; i++) {

    aso_vk_queue_families families = aso_vk_get_queue_families(scratch, devices[i], device->surface);
    bool extensions_supported = aso_vk_check_device_extension_support(scratch, devices[i]);

    // skip further checks if needed extensions are not supported
    if (!extensions_supported) continue;

    aso_vk_surface_details details = aso_vk_get_surface_details(devices[i], device->surface);
    bool surface_details_ok = details.formats_count > 0 && details.present_modes_count > 0;

    bool device_is_suitable = families.has_graphics_family
                           && families.has_present_family
                           && extensions_supported
                           && surface_details_ok;

    if (device_is_suitable) {
      device->physical_device = devices[i];
      device->queue_families = families;
      device->surface_details = details;
      break;
    }
  }

  if (device->physical_device == VK_NULL_HANDLE) {
    LOG("Failed to find suitable GPU");
  }
}

// REGION: LOGICAL DEVICE

void aso_vk_create_logical_device(aso_vk_device *device) {
  ASSERT(device != NULL);

  // use a bitmap to produce an array of unique VkDeviceQueueCreateInfo
  // NOTE: max 8 unique queue families
  u8 unique_families = 0;
  u8 unique_family_count = 0;
  unique_families |= (1 << device->queue_families.graphics_family);
  unique_families |= (1 << device->queue_families.present_family);

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

  VkDeviceCreateInfo device_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = unique_family_count,
    .pQueueCreateInfos = queue_create_infos,
    .enabledExtensionCount = ASO_VK_DEVICE_EXTENSION_COUNT,
    .ppEnabledExtensionNames = aso_vulkan_device_extensions,
    .pEnabledFeatures = &device_features,
  };

  // validation layers are already defined at instance level
  // this is not necessary for up-to-date Vulkan SDK, but included for combatibility
  if (device->use_validation) {
    device_create_info.enabledLayerCount = ASO_VK_VALIDATION_LAYER_COUNT;
    device_create_info.ppEnabledLayerNames = aso_vulkan_validation_layers; 
  } else {
    device_create_info.enabledLayerCount = 0;
  }

  ASO_VK_CHECK(vkCreateDevice(device->physical_device, &device_create_info, NULL, &device->device), "Failed to create logical device");

  vkGetDeviceQueue(device->device, device->queue_families.graphics_family, 0, &device->graphics_queue);
  vkGetDeviceQueue(device->device, device->queue_families.present_family, 0, &device->presentation_queue);
}

// REGION: OTHER

aso_vk_queue_families aso_vk_get_queue_families(aso_arena *scratch, VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  ASSERT(scratch != NULL);
  aso_vk_queue_families indices = {};

  u32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
  VkQueueFamilyProperties *queue_families = ASO_ARENA_ALLOC_ARRAY(scratch, VkQueueFamilyProperties, queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

  for (u32 i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
      indices.has_graphics_family = true;
    }

    // TODO: query SDL for presentation support?
    VkBool32 presentation_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentation_support);
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

aso_vk_surface_details aso_vk_get_surface_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  ASSERT(physical_device != NULL);
  ASSERT(surface != NULL);

  aso_vk_surface_details details = {
    .formats_count = ASO_VK_MAX_SURFACE_FORMATS,
    .present_modes_count = ASO_VK_MAX_PRESENT_MODES,
  };

  // formats
  //vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &details.formats_count, NULL);
  VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &details.formats_count, details.formats);
  ASSERT (result == VK_SUCCESS || result == VK_INCOMPLETE);
  if (result == VK_INCOMPLETE) {
    LOG("Too many surface formats (%u > %u)", details.formats_count, ASO_VK_MAX_SURFACE_FORMATS);
  }

  // present modes
  //vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &details.present_modes_count, NULL);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &details.present_modes_count, details.present_modes);
  ASSERT (result == VK_SUCCESS || result == VK_INCOMPLETE);
  if (result == VK_INCOMPLETE) {
    LOG("Too many present modes (%u > %u)", details.present_modes_count, ASO_VK_MAX_PRESENT_MODES);
  }
  return details;
}

void aso_vk_device_cleanup(aso_vk_device *device) {
  ASSERT(device != NULL);

  // NOTE: physical_device and queues are cleaned up implicitly
  vkDestroyDevice(device->device, NULL);
  vkDestroySurfaceKHR(device->instance, device->surface, NULL);
  vkDestroyInstance(device->instance, NULL);
}
