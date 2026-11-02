#include "rhi/shader.h"

#include "core/types.h"
#include "rhi/device.h"
#include "utils/assert.h"

#include <fstream>
#include <vector>

namespace gn::rhi {

Shader::~Shader() {
    shutdown();
}

Result<void> Shader::load(Device& device, const std::string& path) {
    shutdown();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return Err{Error{"Cannot open shader: " + path}};
    const auto bytes = static_cast<size_t>(file.tellg());
    if (bytes == 0 || bytes % 4 != 0) return Err{Error{"Not a SPIR-V file: " + path}};

    // u32 storage: vkCreateShaderModule needs 4-byte alignment.
    std::vector<u32> code(bytes / 4);
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(bytes))) {
        return Err{Error{"Cannot read shader: " + path}};
    }

    VkShaderModuleCreateInfo ci{};
    ci.sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize  = bytes;
    ci.pCode     = code.data();
    VkResult res = vkCreateShaderModule(device.logical_device(), &ci, nullptr, &m_module);
    if (res != VK_SUCCESS) {
        m_module = VK_NULL_HANDLE;
        return Err{Error{"vkCreateShaderModule failed for " + path}};
    }
    m_device = &device;
    return Ok();
}

void Shader::shutdown() {
    if (m_module != VK_NULL_HANDLE)
        GN_ASSERT(m_device->logical_device() != VK_NULL_HANDLE, "shut down after the Device");
    if (m_module != VK_NULL_HANDLE) vkDestroyShaderModule(m_device->logical_device(), m_module, nullptr);
    m_module = VK_NULL_HANDLE;
}

} // namespace gn::rhi
