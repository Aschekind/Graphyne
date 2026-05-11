#include "rhi/shader.h"

#include "rhi/device.h"
#include "utils/logger.h"

#include <fstream>
#include <vector>

namespace gn::rhi {

Result<VkShaderModule> load_shader_module(Device& device, const std::string& spirv_path) {
    std::ifstream f(spirv_path, std::ios::ate | std::ios::binary);
    if (!f) return Err{Error{"Failed to open shader: " + spirv_path}};

    const auto size = static_cast<usize>(f.tellg());
    if (size == 0 || (size % 4) != 0) {
        return Err{Error{"Shader is empty or not multiple-of-4 bytes: " + spirv_path}};
    }

    std::vector<u8> buffer(size);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));

    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = size;
    ci.pCode    = reinterpret_cast<const u32*>(buffer.data());

    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device.logical_device(), &ci, nullptr, &mod) != VK_SUCCESS) {
        return Err{Error{"vkCreateShaderModule failed for " + spirv_path}};
    }
    GN_INFO("Loaded shader: {} ({} bytes)", spirv_path, size);
    return mod;
}

} // namespace gn::rhi
