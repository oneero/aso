#ifndef ASO_GPU_PIPELINE_H
#define ASO_GPU_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "gpu/gpu_device.h"
#include "math.h"
#include "mem.h"

struct aso_vk_ctx;

struct aso_vk_pipeline
{
  VkPipelineLayout layout;
  VkPipeline       graphics_pipeline;

  // TODO: consider where these resources should live
  u32              vertex_count;
  VkBuffer         vertex_buffer;
  VkDeviceMemory   vertex_buffer_memory;
};

// TODO: input struct for pipeline creation
void           aso_vk_create_graphics_pipeline(aso_arena *scratch, aso_vk_pipeline *pipeline, VkDevice device, VkRenderPass render_pass);
VkShaderModule aso_vk_create_shader_module(VkDevice device, u8 *shader_code, size_t code_size);
void           aso_vk_pipeline_cleanup(aso_vk_pipeline *pipeline, const aso_vk_device *device);

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

aso_vk_vertex_descriptions aso_vk_create_vertex_descriptions(void);
void                       aso_vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const aso_vk_device *device, VkBuffer *buffer, VkDeviceMemory *buffer_memory);
void                       aso_vk_copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
void                       aso_vk_create_vertex_buffer(aso_vk_vertex *vertices, u32 vertex_count, aso_vk_ctx *ctx);
u32                        aso_vk_get_memory_type_index(u32 type_filter, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties *memory_properties);

#endif // ASO_GPU_PIPELINE_H
