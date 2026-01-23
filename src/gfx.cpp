#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "core.h"
#include "gfx.h"
#include "mem.h"
#include "window.h"

// TODO: replace exit()s

const char* aso_vulkan_validation_layers[ASO_VULKAN_VALIDATION_LAYER_COUNT] = {
	"VK_LAYER_KHRONOS_validation"
};

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  aso_create_vulkan_instance(&vulkan_ctx->instance);
  if (!aso_create_vulkan_surface(g_ctx->window.handle, vulkan_ctx)) {
    aso_log("Failed to create SDL3 Vulkan surface\n");
    exit(1);
  }
  aso_select_physical_device(vulkan_ctx);
  aso_create_vulkan_logical_device(vulkan_ctx);
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
  VK_CHECK(vkCreateInstance(&instance_create_info, NULL, instance), "Failed to create Vulkan instance\n");
  
  g_ctx->scratch->offset = scratch_save;
}

VkExtensionProperties* aso_get_available_vulkan_extensions(aso_arena *arena, u32 *count) {
  assert(arena != NULL);
  assert(count != NULL);

  VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, count, NULL), "Failed to get available Vulkan extension count\n");

  VkExtensionProperties *extensions = ASO_ARENA_ALLOC_ARRAY(g_ctx->scratch, VkExtensionProperties, *count);

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

  u32 device_count = 0;
  vkEnumeratePhysicalDevices(vulkan_ctx->instance, &device_count, nullptr);
  if (device_count == 0) {
    aso_log("Failed to find any devices supporting Vulkan\n");
    exit(1);
  }

  size_t scratch_save = g_ctx->scratch->offset;
  VkPhysicalDevice *devices = ASO_ARENA_ALLOC_ARRAY(g_ctx->scratch, VkPhysicalDevice, device_count);

  VK_CHECK(vkEnumeratePhysicalDevices(vulkan_ctx->instance, &device_count, devices), "Failed to enumerate GPUs");

  for (int i = 0; i < device_count; i++) {
    if (aso_is_device_suitable(vulkan_ctx, devices[i])) {
      vulkan_ctx->physical_device = devices[i];
      break;
    }
  }

  if (vulkan_ctx->physical_device == VK_NULL_HANDLE) {
    aso_log("Failed to find suitable GPU\n");
  }

  g_ctx->scratch->offset = scratch_save;
}

bool aso_is_device_suitable(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device) {
  assert(vulkan_ctx != NULL);
  assert(physical_device != NULL);

  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;

  vkGetPhysicalDeviceProperties(physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device_features);

  aso_log(" %s\n", device_properties.deviceName);
  
  aso_vulkan_queue_family_indices indices = aso_get_vulkan_family_indices(vulkan_ctx, physical_device);

  return indices.has_graphics_family;

  //return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader;
}

aso_vulkan_queue_family_indices aso_get_vulkan_family_indices(aso_vulkan_ctx *vulkan_ctx, VkPhysicalDevice physical_device) {
  assert(vulkan_ctx != NULL);

  // NOTE: do we want scratch save here?

  aso_vulkan_queue_family_indices indices = {};

  u32 queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
  VkQueueFamilyProperties *queue_families = ASO_ARENA_ALLOC_ARRAY(g_ctx->scratch, VkQueueFamilyProperties, queue_family_count);
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

// REGION: LOGICAL DEVICE

void aso_create_vulkan_logical_device(aso_vulkan_ctx *vulkan_ctx) {
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

  // INFO: dont need any for now
  device_create_info.enabledExtensionCount = 0;

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

void aso_cleanup_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  vkDestroyDevice(vulkan_ctx->device, nullptr);
  vkDestroySurfaceKHR(vulkan_ctx->instance, vulkan_ctx->surface, nullptr);
  vkDestroyInstance(vulkan_ctx->instance, nullptr);
  // NOTE: physical_device and queues are cleaned up implicitly
}
