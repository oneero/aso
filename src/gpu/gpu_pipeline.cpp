#include <cstddef>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "gpu/gpu_device.h"
#include "io.h"
#include "gpu.h"
#include "gpu_pipeline.h"
#include "gpu_scene.h"

void aso_vk_create_graphics_pipeline(aso_arena *scratch, aso_vk_pipeline *pipeline, VkDevice device, VkRenderPass render_pass, VkDescriptorSetLayout *descriptor_set_layout) {
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

  aso_vk_vertex_descriptions desc = aso_vk_get_vertex_descriptions();

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
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,

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

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = descriptor_set_layout,
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
}
