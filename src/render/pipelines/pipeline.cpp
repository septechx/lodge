#include "pipeline.hpp"

#include "src/render/utils.hpp"
#include "src/utils.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <filesystem>

void loadShader(VkDevice device, const std::filesystem::path path,
                VkShaderModule &module) {
  if (auto spir = readFileToString(path); spir.has_value()) {
    VkShaderModuleCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spir->size(),
        .pCode = reinterpret_cast<const uint32_t *>(spir->data()),
    };
    CHECK_VK(vkCreateShaderModule(device, &sci, nullptr, &module),
             "create shader module");
  } else {
    spdlog::error("Failed to read shader {}", path.string());
    exit(1);
  }
}

ShaderModules loadShaders(VkDevice device, const std::filesystem::path &vert,
                          const std::filesystem::path &frag) {
  ShaderModules modules;
  loadShader(device, vert, modules.vert);
  loadShader(device, frag, modules.frag);
  return modules;
}

void loadShaderFromWords(VkDevice device, std::span<const uint32_t> words,
                         VkShaderModule &module) {
  VkShaderModuleCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = words.size_bytes(),
      .pCode = words.data(),
  };
  CHECK_VK(vkCreateShaderModule(device, &sci, nullptr, &module),
           "create shader module");
}

ShaderModules loadShadersFromWords(VkDevice device,
                                   std::span<const uint32_t> vert,
                                   std::span<const uint32_t> frag) {
  ShaderModules modules;
  loadShaderFromWords(device, vert, modules.vert);
  loadShaderFromWords(device, frag, modules.frag);
  return modules;
}
