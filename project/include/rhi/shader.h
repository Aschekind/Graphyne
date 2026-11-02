/**
 * @file rhi/shader.h
 * @brief SPIR-V shader module loaded from a file.
 */
#pragma once

#include "core/result.h"

#include <string>
#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

/// /// Holds a pointer to the Device: shut it down (or destroy it) before the Device.
class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&)                 = delete;
    Shader& operator=(Shader&&)      = delete;

    /// Load a compiled .spv file. Build the path from platform::base_path() + "shaders/<name>.spv";
    /// the working directory is not the executable's directory.
    Result<void> load(Device& device, const std::string& path);
    void         shutdown();

    VkShaderModule module() const { return m_module; }

private:
    Device*        m_device = nullptr;
    VkShaderModule m_module = VK_NULL_HANDLE;
};

} // namespace gn::rhi
