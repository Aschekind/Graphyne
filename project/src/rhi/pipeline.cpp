#include "rhi/pipeline.h"

#include "rhi/device.h"
#include "utils/assert.h"

#include <string>

namespace gn::rhi {

Pipeline::~Pipeline() {
    shutdown();
}

Result<void> Pipeline::initialize(Device& device, const PipelineDesc& desc) {
    shutdown();
    if (desc.vertex == VK_NULL_HANDLE || desc.fragment == VK_NULL_HANDLE || desc.color_format == VK_FORMAT_UNDEFINED) {
        return Err{Error{"Pipeline: shaders and color format are required"}};
    }
    VkDevice dev = device.logical_device();

    VkPipelineLayoutCreateInfo li{};
    li.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    li.setLayoutCount         = static_cast<u32>(desc.set_layouts.size());
    li.pSetLayouts            = desc.set_layouts.data();
    li.pushConstantRangeCount = static_cast<u32>(desc.push_constants.size());
    li.pPushConstantRanges    = desc.push_constants.data();
    if (vkCreatePipelineLayout(dev, &li, nullptr, &m_layout) != VK_SUCCESS) {
        m_layout = VK_NULL_HANDLE;
        return Err{Error{"vkCreatePipelineLayout failed"}};
    }
    m_device = &device;

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = desc.vertex;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = desc.fragment;
    stages[1].pName  = "main";

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount   = static_cast<u32>(desc.vertex_bindings.size());
    vertex_input.pVertexBindingDescriptions      = desc.vertex_bindings.data();
    vertex_input.vertexAttributeDescriptionCount = static_cast<u32>(desc.vertex_attributes.size());
    vertex_input.pVertexAttributeDescriptions    = desc.vertex_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = desc.topology;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blend{};
    blend.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (desc.alpha_blend) {
        blend.blendEnable         = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp        = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alphaBlendOp        = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments    = &blend;

    const VkDynamicState             dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates    = dynamic_states;

    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount    = 1;
    rendering.pColorAttachmentFormats = &desc.color_format;

    VkGraphicsPipelineCreateInfo pi{};
    pi.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pi.pNext               = &rendering;
    pi.stageCount          = 2;
    pi.pStages             = stages;
    pi.pVertexInputState   = &vertex_input;
    pi.pInputAssemblyState = &input_assembly;
    pi.pViewportState      = &viewport;
    pi.pRasterizationState = &raster;
    pi.pMultisampleState   = &msaa;
    pi.pColorBlendState    = &color_blend;
    pi.pDynamicState       = &dynamic;
    pi.layout              = m_layout;

    VkResult res = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pi, nullptr, &m_pipeline);
    if (res != VK_SUCCESS) {
        m_pipeline = VK_NULL_HANDLE;
        shutdown();
        return Err{Error{"vkCreateGraphicsPipelines failed (code " + std::to_string(res) + ")"}};
    }
    return Ok();
}

void Pipeline::shutdown() {
    if (m_pipeline != VK_NULL_HANDLE || m_layout != VK_NULL_HANDLE)
        GN_ASSERT(m_device->logical_device() != VK_NULL_HANDLE, "shut down after the Device");
    if (m_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device->logical_device(), m_pipeline, nullptr);
    if (m_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_device->logical_device(), m_layout, nullptr);
    m_pipeline = VK_NULL_HANDLE;
    m_layout   = VK_NULL_HANDLE;
}

SamplerDescriptors::~SamplerDescriptors() {
    shutdown();
}

Result<void> SamplerDescriptors::initialize(Device& device, u32 max_sets) {
    shutdown();
    VkDevice dev = device.logical_device();
    m_device     = &device;

    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo li{};
    li.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 1;
    li.pBindings    = &binding;
    if (vkCreateDescriptorSetLayout(dev, &li, nullptr, &m_layout) != VK_SUCCESS) {
        m_layout = VK_NULL_HANDLE;
        return Err{Error{"vkCreateDescriptorSetLayout failed"}};
    }

    VkDescriptorPoolSize       size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_sets};
    VkDescriptorPoolCreateInfo pi{};
    pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.maxSets       = max_sets;
    pi.poolSizeCount = 1;
    pi.pPoolSizes    = &size;
    if (vkCreateDescriptorPool(dev, &pi, nullptr, &m_pool) != VK_SUCCESS) {
        m_pool = VK_NULL_HANDLE;
        shutdown();
        return Err{Error{"vkCreateDescriptorPool failed"}};
    }
    return Ok();
}

void SamplerDescriptors::shutdown() {
    if (m_pool != VK_NULL_HANDLE || m_layout != VK_NULL_HANDLE)
        GN_ASSERT(m_device->logical_device() != VK_NULL_HANDLE, "shut down after the Device");
    if (m_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(m_device->logical_device(), m_pool, nullptr);
    if (m_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_device->logical_device(), m_layout, nullptr);
    m_pool   = VK_NULL_HANDLE;
    m_layout = VK_NULL_HANDLE;
}

Result<VkDescriptorSet> SamplerDescriptors::allocate() {
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = m_pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &m_layout;
    VkDescriptorSet set   = VK_NULL_HANDLE;
    VkResult        res   = vkAllocateDescriptorSets(m_device->logical_device(), &ai, &set);
    if (res != VK_SUCCESS) return Err{Error{"vkAllocateDescriptorSets failed (code " + std::to_string(res) + ")"}};
    return set;
}

void SamplerDescriptors::update(VkDescriptorSet set, VkImageView view, VkSampler sampler) {
    VkDescriptorImageInfo info{sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet  w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = set;
    w.descriptorCount = 1;
    w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo      = &info;
    vkUpdateDescriptorSets(m_device->logical_device(), 1, &w, 0, nullptr);
}

} // namespace gn::rhi
