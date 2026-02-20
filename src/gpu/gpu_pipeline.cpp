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
  
  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName = "main";
  vert_shader_stage_info.pSpecializationInfo = NULL; // no constants

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName = "main";
  frag_shader_stage_info.pSpecializationInfo = NULL; // no constants

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  // vertex input

  aso_vk_vertex_descriptions desc = aso_vk_create_vertex_descriptions();

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions = &desc.binding; // optional
  vertex_input_info.vertexAttributeDescriptionCount = desc.attribute_count;
  vertex_input_info.pVertexAttributeDescriptions = desc.attributes; // optional

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
  multisampling.pSampleMask = NULL; // optional
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
  pipeline_layout_info.pSetLayouts = NULL; // optional
  pipeline_layout_info.pushConstantRangeCount = 0; // optional
  pipeline_layout_info.pPushConstantRanges = NULL; // optional

  ASO_VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline->layout), "Failed to create pipeline layout");

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;

  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = NULL; // optional
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = &dynamic_state;

  pipeline_info.layout = pipeline->layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass = 0;

  // if using derivative pipeline..
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // optional
  pipeline_info.basePipelineIndex = -1; // optional
 
  ASO_VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline->graphics_pipeline), "Failed to create graphics pipeline");

  vkDestroyShaderModule(device, frag_shader_module, NULL);
  vkDestroyShaderModule(device, vert_shader_module, NULL);
}

VkShaderModule aso_vk_create_shader_module(VkDevice device, u8 *shader_code, size_t code_size) {
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code_size;
  create_info.pCode = (const u32 *)shader_code;
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

void aso_vk_create_vertex_buffer(aso_vk_vertex *vertices, u32 vertex_count, const aso_vk_device *device, aso_vk_pipeline *pipeline) {
  ASSERT(vertices != NULL);
  ASSERT(vertex_count > 0);
  ASSERT(pipeline != NULL);
  ASSERT(device != NULL);

  // define buffer
  
  VkBufferCreateInfo buffer_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = sizeof(aso_vk_vertex) * vertex_count,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE
  };

  ASO_VK_CHECK(vkCreateBuffer(device->device, &buffer_info, NULL, &pipeline->vertex_buffer), "Failed to create vertex buffer");

  // define and allocate memory

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(device->device, pipeline->vertex_buffer, &memory_requirements);

  VkMemoryAllocateInfo alloc_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = aso_vk_get_memory_type_index(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &device->memory_properties)
  };

  ASO_VK_CHECK(vkAllocateMemory(device->device, &alloc_info, NULL, &pipeline->vertex_buffer_memory), "Failed to allocate vertex buffer memory");

  // bind buffer to memory

  vkBindBufferMemory(device->device, pipeline->vertex_buffer, pipeline->vertex_buffer_memory, 0);

  // map to host memory, copy and unmap
  // NOTE: driver might not copy immediately to device 
  // NOTE: we use VK_MEMORY_PROPERTY_HOST_COHERENT_BIT to ensure memory sync with driver

  void* data;
  vkMapMemory(device->device, pipeline->vertex_buffer_memory, 0, buffer_info.size, 0, &data);
  memcpy(data, vertices, (size_t) buffer_info.size);
  vkUnmapMemory(device->device, pipeline->vertex_buffer_memory);

  pipeline->vertex_count = vertex_count;
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
