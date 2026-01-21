#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "core.h"
#include "gfx.h"
#include "window.h"
#include "mem.h"

// TODO: replace exit()s

const char* aso_vulkan_validation_layers[ASO_VULKAN_VALIDATION_LAYER_COUNT] = {
	"VK_LAYER_KHRONOS_validation"
};

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  aso_create_vulkan_instance(&vulkan_ctx->instance);
  aso_select_physical_device(vulkan_ctx);
}

// REGION: INSTANCE

void aso_create_vulkan_instance(VkInstance *instance) {
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

  // TODO: add dedicated arenas to vulkan_ctx
  size_t scratch_save = g_ctx->scratch->offset;

  // REGION: LAYERS

  // list avaiable layers
  u32 available_layer_count = 0;
  VkLayerProperties *layers = aso_get_available_vulkan_layers(g_ctx->scratch, &available_layer_count);
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
  VkExtensionProperties *extensions = aso_get_available_vulkan_extensions(g_ctx->scratch, &available_extension_count); 
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
  if (vkCreateInstance(&instance_create_info, NULL, instance) != VK_SUCCESS) {
    aso_log("Failed to create Vulkan instance\n");
    exit(1);
  }
  
  g_ctx->scratch->offset = scratch_save;
}

VkExtensionProperties* aso_get_available_vulkan_extensions(aso_arena *arena, u32 *count) {
  assert(arena != NULL);
  assert(count != NULL);

  if (vkEnumerateInstanceExtensionProperties(NULL, count, NULL) != VK_SUCCESS) {
    aso_log("Failed to get available Vulkan extension count\n");
    exit(1);
  }

  VkExtensionProperties *extensions = aso_arena_alloc_array(g_ctx->scratch, VkExtensionProperties, *count);
  if (extensions == NULL) {
    aso_log("Failed to allocate memory\n");
    exit(1);
  }

  if (vkEnumerateInstanceExtensionProperties(NULL, count, extensions) != VK_SUCCESS) {
    aso_log("Failed to enumerate available Vulkan extensions\n");
    exit(1);
  }
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

  if (vkEnumerateInstanceLayerProperties(count, NULL) != VK_SUCCESS) {
    aso_log("Failed to get available Vulkan layer count\n");
    exit(1);
  }

  VkLayerProperties *layers = aso_arena_alloc_array(arena, VkLayerProperties, *count);
  if (layers == NULL) {
    aso_log("Failed to allocate memory\n");
    exit(1);
  }

  if (vkEnumerateInstanceLayerProperties(count, layers) != VK_SUCCESS) {
    aso_log("Failed to enumerate available Vulkan layers\n");
    exit(1);
  }

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
  if (aso_enable_vulkan_validation_layers)
  {
    *count += ASO_VULKAN_VALIDATION_LAYER_COUNT;
    return aso_vulkan_validation_layers;
  }
  return NULL;
}

// REGION: PHYSICAL DEVICE

void aso_select_physical_device(aso_vulkan_ctx *vulkan_ctx) {
  assert(vulkan_ctx != NULL);

  vulkan_ctx->physical_device = VK_NULL_HANDLE;

  u32 device_count = 0;
  vkEnumeratePhysicalDevices(vulkan_ctx->instance, &device_count, nullptr);
  if (device_count == 0) {
    aso_log("Failed to find any devices supporting Vulkan\n");
    exit(1);
  }

  size_t scratch_save = g_ctx->scratch->offset;
  VkPhysicalDevice *devices = aso_arena_alloc_array(g_ctx->scratch, VkPhysicalDevice, device_count);

  vkEnumeratePhysicalDevices(vulkan_ctx->instance, &device_count, devices);

  for (int i = 0; i < device_count; i++) {
    if (aso_is_device_suitable(devices[i])) {
      vulkan_ctx->physical_device = devices[i];
      break;
    }
  }

  if (vulkan_ctx->physical_device == VK_NULL_HANDLE) {
    aso_log("Failed to find suitable GPU\n");
  }

  g_ctx->scratch->offset = scratch_save;
}

// TODO: implement proper checks / gpu selection procedure
bool aso_is_device_suitable(VkPhysicalDevice device) {
  assert(device != NULL);

  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;

  // TODO: check if these fail
  vkGetPhysicalDeviceProperties(device, &device_properties);
  vkGetPhysicalDeviceFeatures(device, &device_features);

  aso_log(" %s\n", device_properties.deviceName);
  
  aso_vulkan_queue_family_indices indices = aso_get_vulkan_family_indices(device);

  return indices.has_graphics_family;

  //return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader;
}

aso_vulkan_queue_family_indices aso_get_vulkan_family_indices(VkPhysicalDevice device) {
  assert(device != NULL);

  // NOTE: do we want scratch save here?

  aso_vulkan_queue_family_indices indices = {};

  u32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
  VkQueueFamilyProperties *queue_families = aso_arena_alloc_array(g_ctx->scratch, VkQueueFamilyProperties, queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
  
  for (int i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
      indices.has_graphics_family = true;
    }

    // TODO: check for presentation

    if (indices.has_graphics_family) {
      break;
    }
  }

  return indices;
}

void aso_cleanup_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  vkDestroyInstance(vulkan_ctx->instance, nullptr);
}
