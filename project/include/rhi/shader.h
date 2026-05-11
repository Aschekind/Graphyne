/**
 * @file rhi/shader.h
 * @brief VkShaderModule loading from a SPIR-V file on disk.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <string>
#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

/// Load a SPIR-V binary file and create a VkShaderModule. Caller owns the
/// returned handle and must destroy it with vkDestroyShaderModule.
Result<VkShaderModule> load_shader_module(Device& device, const std::string& spirv_path);

} // namespace gn::rhi
