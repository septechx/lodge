#include "ssr.hpp"

#include "src/render/pipelines/embedded_shaders.hpp"
#include "src/render/utils.hpp"

ComputePipeline createSsrPipeline(VkDevice device,
                                  VkDescriptorSetLayout frameLayout,
                                  VkDescriptorSetLayout passLayout) {
  VkShaderModule comp;
  loadShaderFromWords(device, LODGE_SHADER_WORDS(k_ssrComp), comp);
  VkPipelineShaderStageCreateInfo stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = comp,
      .pName = "main"};
  VkDescriptorSetLayout setLayouts[2] = {frameLayout, passLayout};
  VkPipelineLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 2,
      .pSetLayouts = setLayouts,
  };
  VkPipelineLayout layout;
  CHECK_VK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout),
           "create ssr pipeline layout");
  VkComputePipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = stage,
      .layout = layout};
  VkPipeline pipeline;
  CHECK_VK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &info, nullptr,
                                    &pipeline),
           "create ssr compute pipeline");
  vkDestroyShaderModule(device, comp, nullptr);
  return ComputePipeline{pipeline, layout};
}
