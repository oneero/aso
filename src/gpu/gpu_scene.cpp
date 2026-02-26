#include <cstddef>
#include <vulkan/vulkan_core.h>

#include "aso.h"
#include "base.h"
#include "gpu.h"
#include "gpu/gpu_config.h"
#include "gpu/gpu_device.h"
#include "math.h"
#include "gpu_scene.h"

void aso_vk_scene_init(aso_vk_ctx *ctx) {
  aso_vk_vertex vertices[] = {
      {.pos = {.x = -0.5f, .y = 0.0f, .z = -0.5f}, .color = {.x = 1.0f, .y = 0.0f, .z = 0.0f}},
      {.pos = {.x =  0.5f, .y = 0.0f, .z = -0.5f}, .color = {.x = 0.0f, .y = 1.0f, .z = 0.0f}},
      {.pos = {.x =  0.5f, .y = 0.0f, .z =  0.5f}, .color = {.x = 0.0f, .y = 0.0f, .z = 1.0f}},
      {.pos = {.x = -0.5f, .y = 0.0f, .z =  0.5f}, .color = {.x = 1.0f, .y = 1.0f, .z = 1.0f}},
  };
  aso_vk_create_vertex_buffer(vertices, 4, ctx);

  // counter-clockwise
  u16 indices[] = { 2, 1, 0, 2, 0, 3 };
  aso_vk_create_index_buffer(indices, 6, ctx);

  aso_vk_create_uniform_buffers(&ctx->scene, &ctx->device);

  aso_vk_create_descriptor_set_layout(&ctx->scene, &ctx->device);
  aso_vk_create_descriptor_pool(&ctx->scene, &ctx->device);
  aso_vk_create_descriptor_sets(&ctx->scene, &ctx->device);
}

void aso_vk_scene_cleanup(aso_vk_scene *scene, const aso_vk_device *device) {
  ASSERT(scene != NULL);
  ASSERT(device != NULL);

  vkDestroyBuffer(device->device, scene->vertex_buffer, NULL);
  vkFreeMemory(device->device, scene->vertex_buffer_memory, NULL);
  vkDestroyBuffer(device->device, scene->index_buffer, NULL);
  vkFreeMemory(device->device, scene->index_buffer_memory, NULL);

  for (size_t i = 0; i < ASO_VK_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device->device, scene->uniform_buffers[i], NULL);
    vkFreeMemory(device->device, scene->uniform_buffers_memory[i], NULL);
  }

  // sets are destroyed implicitly
  vkDestroyDescriptorPool(device->device, scene->descriptor_pool, NULL);
  vkDestroyDescriptorSetLayout(device->device, scene->descriptor_set_layout, NULL);
}

// REGION: BUFFERS

void aso_vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const aso_vk_device *device, VkBuffer *buffer, VkDeviceMemory *buffer_memory) {
  ASSERT(device != NULL);
  ASSERT(buffer != NULL);
  ASSERT(buffer_memory != NULL);

  // define buffer
  VkBufferCreateInfo buffer_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE
  };
  ASO_VK_CHECK(vkCreateBuffer(device->device, &buffer_info, NULL, buffer), "Failed to create vertex buffer");
  
  // define and allocate memory

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(device->device, *buffer, &memory_requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = aso_vk_get_memory_type_index(memory_requirements.memoryTypeBits, properties, &device->memory_properties)
  };

  ASO_VK_CHECK(vkAllocateMemory(device->device, &alloc_info, NULL, buffer_memory), "Failed to allocate vertex buffer memory");
  
  // bind buffer to memory

  vkBindBufferMemory(device->device, *buffer, *buffer_memory, 0);
}

u32 aso_vk_get_memory_type_index(u32 type_filter, VkMemoryPropertyFlags properties, const VkPhysicalDeviceMemoryProperties *memory_properties) {
  for (u32 i = 0; i < memory_properties->memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) && (memory_properties->memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  LOG_ERROR("Failed to find suitable memory type");
  return 0;
}

void aso_vk_copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, VkCommandBuffer command_buffer) {
  VkBufferCopy copy_region = {
    .srcOffset = 0, // optional
    .dstOffset = 0, // optional
    .size = size,
  };

  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
}

// REGION: VERTEX

aso_vk_vertex_descriptions aso_vk_get_vertex_descriptions(void) {
  return aso_vk_vertex_descriptions {
    .binding = { // VkVertexInputBindingDescription
      .binding = 0,
      .stride = sizeof(aso_vk_vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    },
    .attributes = { // VkVertexInputAttributeDescription
  { // pos
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(aso_vk_vertex, pos)
      },
  { // color
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(aso_vk_vertex, color)
      }
    },
    .attribute_count = 2
  };
}


void aso_vk_create_vertex_buffer(aso_vk_vertex *vertices, u32 vertex_count, aso_vk_ctx *ctx) {
  ASSERT(vertices != NULL);
  ASSERT(vertex_count > 0);
  ASSERT(ctx != NULL);

  VkDeviceSize buffer_size = sizeof(vertices[0]) * vertex_count;

  // staging buffer for data transfer
  
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  aso_vk_create_buffer(buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &ctx->device,
                       &staging_buffer,
                       &staging_buffer_memory);

  // map staging buffer to host memory, copy and unmap
  // NOTE: driver might not copy immediately to device 
  // NOTE: we use VK_MEMORY_PROPERTY_HOST_COHERENT_BIT to ensure memory sync with driver

  void* data;
  vkMapMemory(ctx->device.device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, vertices, (size_t) buffer_size);
  vkUnmapMemory(ctx->device.device, staging_buffer_memory);

  // device local vertex buffer (and destination of transfer)

  aso_vk_create_buffer(buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &ctx->device,
                       &ctx->scene.vertex_buffer,
                       &ctx->scene.vertex_buffer_memory);

  // copy from staging
  aso_vk_immediate_begin(ctx);
  aso_vk_copy_buffer(staging_buffer, ctx->scene.vertex_buffer, buffer_size, ctx->immediate_cmd_buffer);
  aso_vk_immediate_end(ctx);

  // destroy staging
  vkDestroyBuffer(ctx->device.device, staging_buffer, NULL);
  vkFreeMemory(ctx->device.device, staging_buffer_memory, NULL);

  ctx->scene.vertex_count = vertex_count;
}

void aso_vk_create_index_buffer(u16 *indices, u32 index_count, aso_vk_ctx *ctx) {
  ASSERT(indices != NULL);
  ASSERT(index_count > 0);
  ASSERT(ctx != NULL);

  VkDeviceSize buffer_size = sizeof(indices[0]) * index_count;

  // staging buffer for data transfer
  
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  aso_vk_create_buffer(buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &ctx->device,
                       &staging_buffer,
                       &staging_buffer_memory);

  // map staging buffer to host memory, copy and unmap

  void* data;
  vkMapMemory(ctx->device.device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, indices, (size_t) buffer_size);
  vkUnmapMemory(ctx->device.device, staging_buffer_memory);

  // device local vertex buffer (and destination of transfer)

  aso_vk_create_buffer(buffer_size,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       &ctx->device,
                       &ctx->scene.index_buffer,
                       &ctx->scene.index_buffer_memory);

  // copy from staging
  aso_vk_immediate_begin(ctx);
  aso_vk_copy_buffer(staging_buffer, ctx->scene.index_buffer, buffer_size, ctx->immediate_cmd_buffer);
  aso_vk_immediate_end(ctx);

  // destroy staging
  vkDestroyBuffer(ctx->device.device, staging_buffer, NULL);
  vkFreeMemory(ctx->device.device, staging_buffer_memory, NULL);

  ctx->scene.index_count = index_count;
}

// REGION: UBOs

void aso_vk_create_uniform_buffers(aso_vk_scene *scene, const aso_vk_device *device) {
  ASSERT(scene != NULL);
  ASSERT(device != NULL);

  VkDeviceSize buffer_size = sizeof(aso_vk_ubo);

  // no staging as the buffers are updated every frame

  for (size_t i = 0; i < ASO_VK_FRAMES_IN_FLIGHT; i++) {
    aso_vk_create_buffer(buffer_size,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         device,
                         &scene->uniform_buffers[i],
                         &scene->uniform_buffers_memory[i]);

    vkMapMemory(device->device, scene->uniform_buffers_memory[i], 0, buffer_size, 0, &scene->uniform_buffers_mapped[i]);
  }
}

void aso_vk_update_uniform_buffers(u32 current_frame, const aso_vk_scene *scene, VkExtent2D extent) {
  ASSERT(scene != NULL);

  f32 time = (f32)aso_clock_elapsed_s(&g_ctx->app_clock);

  aso_vk_ubo ubo = {};
  ubo.model = aso_rot_m4(time * aso_rad(90.0f), v3f32 {.x = 0.0f, .y = 1.0f, .z = 0.0f});
  ubo.view = aso_lookat(v3f32 {.x = 2.0f, .y = 2.0f, .z = 2.0f}, v3f32 {.x = 0.0f, .y = 0.0f, .z = 0.0f}, v3f32 {.x = 0.0f, .y = 1.0f, .z = 0.0f});
  ubo.proj = aso_perspective(aso_rad(45.0f), (f32)extent.width / (f32)extent.height, 0.1f, 10.0f);

  memcpy(scene->uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
}

// REGION: DESCRIPTORS

void aso_vk_create_descriptor_set_layout(aso_vk_scene *scene, const aso_vk_device *device) {
  ASSERT(scene != NULL);
  ASSERT(device != NULL);

  VkDescriptorSetLayoutBinding ubo_layout_binding = {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = NULL, // optional
  };

  VkDescriptorSetLayoutCreateInfo layout_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &ubo_layout_binding,
  };

  ASO_VK_CHECK(vkCreateDescriptorSetLayout(device->device, &layout_info, NULL, &scene->descriptor_set_layout), "Failed to create descriptor set layout");
}

void aso_vk_create_descriptor_pool(aso_vk_scene *scene, const aso_vk_device *device) {
  ASSERT(scene != NULL);
  ASSERT(device != NULL);

  VkDescriptorPoolSize pool_size = {
    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = ASO_VK_FRAMES_IN_FLIGHT
  };

  VkDescriptorPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = ASO_VK_FRAMES_IN_FLIGHT,
    .poolSizeCount = 1,
    .pPoolSizes = &pool_size,
  };

  ASO_VK_CHECK(vkCreateDescriptorPool(device->device, &pool_info, NULL, &scene->descriptor_pool), "Failed to create descriptor pool");
}

void aso_vk_create_descriptor_sets(aso_vk_scene *scene, const aso_vk_device *device) {
  ASSERT(scene != NULL);
  ASSERT(device != NULL);

  // allocate descriptor sets

  // Vulkan wants an array with a copy of our layout for each set we want to create
  VkDescriptorSetLayout layouts[ASO_VK_FRAMES_IN_FLIGHT];
  for (size_t i = 0; i < ASO_VK_FRAMES_IN_FLIGHT; i++) {
    layouts[i] = scene->descriptor_set_layout;
  }

  VkDescriptorSetAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = scene->descriptor_pool,
    .descriptorSetCount = ASO_VK_FRAMES_IN_FLIGHT,
    .pSetLayouts = layouts
  };

  ASO_VK_CHECK(vkAllocateDescriptorSets(device->device, &alloc_info, scene->descriptor_sets), "Failed to allocate descriptor sets");

  // configure descriptor sets

  for (size_t i = 0; i < ASO_VK_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo buffer_info = {
      .buffer = scene->uniform_buffers[i],
      .offset = 0,
      .range = sizeof(aso_vk_ubo)
    };

    VkWriteDescriptorSet desc_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = scene->descriptor_sets[i],
      .dstBinding = 0,
      .dstArrayElement = 0,

      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,

      .pImageInfo = NULL, // optional
      .pBufferInfo = &buffer_info,
      .pTexelBufferView = NULL, // optional
    };

    vkUpdateDescriptorSets(device->device, 1, &desc_write, 0, NULL);
  }
}

