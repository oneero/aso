#include <cassert>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "core.h"
#include "gfx.h"
#include "window.h"
#include "mem.h"

void aso_init_vulkan(aso_vulkan_ctx *vulkan_ctx) {
  aso_create_vulkan_instance(&vulkan_ctx->instance);
}

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

  // list available extensions
  size_t scratch_save = g_ctx->scratch->offset;
  u32 extensions_available_count = 0;
  if(vkEnumerateInstanceExtensionProperties(NULL, &extensions_available_count, NULL) != VK_SUCCESS) {
    aso_log("Failed to get available Vulkan extension count\n");
    exit(1);
  }

  VkExtensionProperties *extensions = aso_arena_alloc_array(g_ctx->scratch, VkExtensionProperties, extensions_available_count);
  if (extensions == NULL) {
    aso_log("OOM\n");
    exit(1);
  }

  if (vkEnumerateInstanceExtensionProperties(NULL, &extensions_available_count, extensions) != VK_SUCCESS) {
    aso_log("Failed to enumerate available Vulkan extensions\n");
    exit(1);
  }

  aso_log("Available Vulkan extensions:\n");
  for (u32 i = 0; i < extensions_available_count; i++) {
    aso_log(" %s\n", extensions[i].extensionName);
  }

  g_ctx->scratch->offset = scratch_save;

  // get array of extensions and update count
  instance_create_info.ppEnabledExtensionNames = aso_get_vulkan_extensions(&instance_create_info.enabledExtensionCount);

  aso_log("Enabled Vulkan extensions:\n");
  for (int i = 0; i < instance_create_info.enabledExtensionCount; i++) {
    aso_log(" %s\n", instance_create_info.ppEnabledExtensionNames[i]);
  }

  // TODO: validation layers
  instance_create_info.enabledLayerCount = 0;

  // create instance
  // TODO: use an explicit allocator
  if (vkCreateInstance(&instance_create_info, NULL, instance) != VK_SUCCESS) {
    aso_log("Failed to create Vulkan instance\n");
    exit(1);
  }
}

char const * const * aso_get_vulkan_extensions(u32 *count) {
  assert(count != NULL);

  // get extensions from window framework (currently just SDL)
  char const * const * window_extensions = aso_get_window_vulkan_extensions(count);
  if (window_extensions == NULL) {
    aso_log("Failed to fetch window Vulkan extensions\n");
    exit(1);
  }

#ifndef VULKAN_VALIDATION_LAYERS
  return window_extensions;
#endif

  // TODO: insert debug utils extension when using validation layers
  return window_extensions;
}
