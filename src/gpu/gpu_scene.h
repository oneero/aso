#ifndef ASO_GPU_SCENE_H
#define ASO_GPU_SCENE_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "gpu/gpu_config.h"
#include "gpu/gpu_device.h"
#include "math.h"

struct aso_vk_ctx;

struct aso_vk_scene
{
  u32                   vertex_count;
  u32                   index_count;
  VkBuffer              vertex_buffer;
  VkDeviceMemory        vertex_buffer_memory;
  VkBuffer              index_buffer;
  VkDeviceMemory        index_buffer_memory;
  VkBuffer              uniform_buffers[ASO_VK_FRAMES_IN_FLIGHT];
  VkDeviceMemory        uniform_buffers_memory[ASO_VK_FRAMES_IN_FLIGHT];
  void                 *uniform_buffers_mapped[ASO_VK_FRAMES_IN_FLIGHT];

  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool      descriptor_pool;
  VkDescriptorSet       descriptor_sets[ASO_VK_FRAMES_IN_FLIGHT];
};

void aso_vk_scene_init(aso_vk_ctx *ctx);
void aso_vk_scene_cleanup(aso_vk_scene *scene, const aso_vk_device *device);

// REGION: VERTEX

struct aso_vk_vertex
{
  v2f32 pos;
  v3f32 color;
};

struct aso_vk_vertex_descriptions
{
  VkVertexInputBindingDescription   binding;
  VkVertexInputAttributeDescription attributes[2];
  u32                               attribute_count;
};

aso_vk_vertex_descriptions aso_vk_get_vertex_descriptions(void);
void                       aso_vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const aso_vk_device *device, VkBuffer *buffer, VkDeviceMemory *buffer_memory);
void                       aso_vk_copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
void                       aso_vk_create_vertex_buffer(aso_vk_vertex *vertices, u32 vertex_count, aso_vk_ctx *ctx);
void                       aso_vk_create_index_buffer(u16 *indices, u32 index_count, aso_vk_ctx *ctx);
u32                        aso_vk_get_memory_type_index(u32 type_filter, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties *memory_properties);

// REGION: UBO

struct aso_vk_ubo {
  m4f32 model;
  m4f32 view;
  m4f32 proj;
};

void aso_vk_create_uniform_buffers(aso_vk_scene *scene, const aso_vk_device *device);
void aso_vk_update_uniform_buffers(u32 current_frame, const aso_vk_scene *scene, VkExtent2D extent);

// REGION: DESCRIPTORS

void aso_vk_create_descriptor_set_layout(aso_vk_scene *scene, const aso_vk_device *device);
void aso_vk_create_descriptor_pool(aso_vk_scene *scene, const aso_vk_device *device);
void aso_vk_create_descriptor_sets(aso_vk_scene *scene, const aso_vk_device *device);

#endif // ASO_GPU_SCENE_H
