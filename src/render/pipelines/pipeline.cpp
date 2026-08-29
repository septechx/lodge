#include "pipeline.hpp"

#include "src/render/utils.hpp"
#include "src/utils.hpp"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <filesystem>

static void loadShader(VkDevice device, const std::filesystem::path path,
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

ShaderModules loadShaders(VkDevice device) {
  ShaderModules modules;
  loadShader(device, "build/shader.vert.spv", modules.vert);
  loadShader(device, "build/shader.frag.spv", modules.frag);
  return modules;
}
