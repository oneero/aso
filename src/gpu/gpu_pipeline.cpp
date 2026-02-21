#include <cstddef>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "gpu/gpu_device.h"
#include "io.h"
#include "gpu.h"
#include "gpu_pipeline.h"

void aso_vk_create_graphics_pipeline(aso_arena *scratch, aso_vk_pipeline *pipeline, VkDevice device, VkRenderPass render_pass) {
  ASSERT(scratch != NULL);
  ASSERT(pipeline != NULL);
  ASSERT(device != NULL);
  ASSERT(render_pass != NULL);

  // load shader bytecode into shader modules
  size_t vert_shader_code_size = 0;
  size_t frag_shader_code_size = 0;
  u8 *vert_shader_code = aso_read_binary_file(scratch, "assets/vert.spv", &vert_shader_code_size);
  u8 *frag_shader_code = aso_read_binary_file(scratch, "assets/frag.spv", &frag_shader_code_size);
  VkShaderModule vert_shader_module = aso_vk_create_shader_module(device, vert_shader_code, vert_shader_code_size);
  VkShaderModule frag_shader_module = aso_vk_create_shader_module(device, frag_shader_code, frag_shader_code_size);

  // shader stages

  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vert_shader_module,
    .pName = "main",
    .pSpecializationInfo = NULL // no constants
  };

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = frag_shader_module,
    .pName = "main",
    .pSpecializationInfo = NULL, // no constants
  };

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  // vertex input

  aso_vk_vertex_descriptions desc = aso_vk_create_vertex_descriptions();

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &desc.binding, // optional
    .vertexAttributeDescriptionCount = desc.attribute_count,
    .pVertexAttributeDescriptions = desc.attributes, // optional
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  // viewport and scissor as dynamic states to be specified at draw time
  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates = dynamic_states,
  };

  VkPipelineViewportStateCreateInfo viewport_state = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1,
  };

  // rasterizer

  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,

    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f, // optional
    .depthBiasClamp = 0.0f, // optional
    .depthBiasSlopeFactor = 0.0f, // optional
    
    .lineWidth = 1.0f,
  };

  // TODO: enable later
  VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f, // optional
    .pSampleMask = NULL, // optional
    .alphaToCoverageEnable = VK_FALSE, // optional
    .alphaToOneEnable = VK_FALSE, // optional
  };

  // TODO: add depth and stencil states?

  // color blending
  // NOTE:: alpha blending disabled for now

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // optional
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // optional
    .colorBlendOp = VK_BLEND_OP_ADD, // optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // optional
    .alphaBlendOp = VK_BLEND_OP_ADD, // optional
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo color_blending = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY, // optional
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment,
    .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f } // optional
  };

  // pipeline layout for uniforms

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0, // optional
    .pSetLayouts = NULL, // optional
    .pushConstantRangeCount = 0, // optional
    .pPushConstantRanges = NULL, // optional
  };

  ASO_VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline->layout), "Failed to create pipeline layout");

  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shader_stages,

    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = NULL, // optional
    .pColorBlendState = &color_blending,
    .pDynamicState = &dynamic_state,

    .layout = pipeline->layout,
    .renderPass = render_pass,
    .subpass = 0,

    // if using derivative pipeline..
    .basePipelineHandle = VK_NULL_HANDLE, // optional
    .basePipelineIndex = -1, // optional
  };

  ASO_VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline->graphics_pipeline), "Failed to create graphics pipeline");

  vkDestroyShaderModule(device, frag_shader_module, NULL);
  vkDestroyShaderModule(device, vert_shader_module, NULL);
}

VkShaderModule aso_vk_create_shader_module(VkDevice device, u8 *shader_code, size_t code_size) {
  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code_size,
    .pCode = (const u32 *)shader_code,
  };

  VkShaderModule module;
  ASO_VK_CHECK(vkCreateShaderModule(device, &create_info, NULL, &module), "Failed to create shader module");
  return module;
}

void aso_vk_pipeline_cleanup(aso_vk_pipeline *pipeline, const aso_vk_device *device) {
  ASSERT(pipeline != NULL);
  ASSERT(device != NULL);
  
  vkDestroyPipeline(device->device, pipeline->graphics_pipeline, NULL);
  vkDestroyPipelineLayout(device->device, pipeline->layout, NULL);

  vkDestroyBuffer(device->device, pipeline->vertex_buffer, NULL);
  vkFreeMemory(device->device, pipeline->vertex_buffer_memory, NULL);
  vkDestroyBuffer(device->device, pipeline->index_buffer, NULL);
  vkFreeMemory(device->device, pipeline->index_buffer_memory, NULL);
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

aso_vk_vertex_descriptions aso_vk_create_vertex_descriptions(void) {
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
        .format = VK_FORMAT_R32G32_SFLOAT,
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
                       &ctx->pipeline.vertex_buffer,
                       &ctx->pipeline.vertex_buffer_memory);

  // copy from staging
  aso_vk_immediate_begin(ctx);
  aso_vk_copy_buffer(staging_buffer, ctx->pipeline.vertex_buffer, buffer_size, ctx->immediate_cmd_buffer);
  aso_vk_immediate_end(ctx);

  // destroy staging
  vkDestroyBuffer(ctx->device.device, staging_buffer, NULL);
  vkFreeMemory(ctx->device.device, staging_buffer_memory, NULL);

  ctx->pipeline.vertex_count = vertex_count;
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
                       &ctx->pipeline.index_buffer,
                       &ctx->pipeline.index_buffer_memory);

  // copy from staging
  aso_vk_immediate_begin(ctx);
  aso_vk_copy_buffer(staging_buffer, ctx->pipeline.index_buffer, buffer_size, ctx->immediate_cmd_buffer);
  aso_vk_immediate_end(ctx);

  // destroy staging
  vkDestroyBuffer(ctx->device.device, staging_buffer, NULL);
  vkFreeMemory(ctx->device.device, staging_buffer_memory, NULL);

  ctx->pipeline.index_count = index_count;
}
