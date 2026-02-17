#ifndef ASO_GPU_PIPELINE_H
#define ASO_GPU_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "base.h"
#include "mem.h"

struct aso_vk_pipeline
{
  VkPipelineLayout layout;
  VkPipeline       graphics_pipeline;
};

// TODO: input struct for pipeline creation
void           aso_vk_create_graphics_pipeline(aso_arena *scratch, aso_vk_pipeline *pipeline, VkDevice device, VkRenderPass render_pass);
VkShaderModule aso_vk_create_shader_module(VkDevice device, u8 *shader_code, size_t code_size);

#endif // ASO_GPU_PIPELINE_H
