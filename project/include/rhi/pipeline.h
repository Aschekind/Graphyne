/**
 * @file rhi/pipeline.h
 * @brief Graphics pipeline for dynamic rendering, and the descriptor helpers it is used with.
 */
#pragma once

#include "core/result.h"
#include "core/types.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace gn::rhi {

class Device;

struct PipelineDesc {
    VkShaderModule                                 vertex       = VK_NULL_HANDLE;
    VkShaderModule                                 fragment     = VK_NULL_HANDLE;
    VkFormat                                       color_format = VK_FORMAT_UNDEFINED; ///< Swapchain::image_format()
    std::vector<VkVertexInputBindingDescription>   vertex_bindings;                    ///< empty: no vertex buffer
    std::vector<VkVertexInputAttributeDescription> vertex_attributes;
    std::vector<VkDescriptorSetLayout>             set_layouts;
    std::vector<VkPushConstantRange>               push_constants;
    VkPrimitiveTopology                            topology    = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool                                           alpha_blend = false; ///< src-alpha / one-minus-src-alpha
};

/// Pipeline + layout. No culling, no depth; viewport and scissor are dynamic (set them every frame).
/// /// Holds a pointer to the Device: shut it down (or destroy it) before the Device.
class Pipeline {
public:
    Pipeline() = default;
    ~Pipeline();

    Pipeline(const Pipeline&)            = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&)                 = delete;
    Pipeline& operator=(Pipeline&&)      = delete;

    Result<void> initialize(Device& device, const PipelineDesc& desc);
    void         shutdown();

    VkPipeline       handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout; }

private:
    Device*          m_device   = nullptr;
    VkPipelineLayout m_layout   = VK_NULL_HANDLE;
    VkPipeline       m_pipeline = VK_NULL_HANDLE;
};

/// Pool of descriptor sets that each hold one combined image sampler (binding 0, fragment stage).
/// Owns the matching set layout; sets are freed with the pool.
/// /// Holds a pointer to the Device: shut it down (or destroy it) before the Device.
class SamplerDescriptors {
public:
    SamplerDescriptors() = default;
    ~SamplerDescriptors();

    SamplerDescriptors(const SamplerDescriptors&)            = delete;
    SamplerDescriptors& operator=(const SamplerDescriptors&) = delete;
    SamplerDescriptors(SamplerDescriptors&&)                 = delete;
    SamplerDescriptors& operator=(SamplerDescriptors&&)      = delete;

    Result<void> initialize(Device& device, u32 max_sets);
    void         shutdown();

    Result<VkDescriptorSet> allocate();
    /// Point `set` at an image in SHADER_READ_ONLY_OPTIMAL.
    void update(VkDescriptorSet set, VkImageView view, VkSampler sampler);

    VkDescriptorSetLayout layout() const { return m_layout; }

private:
    Device*               m_device = nullptr;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorPool      m_pool   = VK_NULL_HANDLE;
};

} // namespace gn::rhi
