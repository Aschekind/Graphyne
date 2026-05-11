#include "graphics/sprite_renderer.h"

#include "resources/resource_manager.h"
#include "rhi/device.h"
#include "rhi/shader.h"
#include "utils/logger.h"

#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

namespace gn::graphics {

namespace {

struct Vertex {
    f32 pos[2];
    f32 uv[2];
};

// Unit quad ranging from (0,0) to (1,1), with UVs matching.
const Vertex kQuadVertices[4] = {
    {{0.0f, 0.0f}, {0.0f, 0.0f}},  // top-left
    {{1.0f, 0.0f}, {1.0f, 0.0f}},  // top-right
    {{1.0f, 1.0f}, {1.0f, 1.0f}},  // bottom-right
    {{0.0f, 1.0f}, {0.0f, 1.0f}},  // bottom-left
};
const u16 kQuadIndices[6] = {0, 1, 2, 2, 3, 0};

struct PushBlock {
    mat4 model;
    vec4 tint;
};

} // namespace

SpriteRenderer::~SpriteRenderer() { shutdown(); }

Result<void> SpriteRenderer::initialize(rhi::Device& device,
                                        resources::ResourceManager& resources,
                                        VkFormat color_format,
                                        const std::string& shader_dir) {
    m_device    = &device;
    m_resources = &resources;

    if (auto r = create_quad_buffers();        !r) return r;
    if (auto r = create_camera_resources();    !r) return r;
    if (auto r = create_pipeline(color_format, shader_dir); !r) return r;
    GN_INFO("SpriteRenderer initialized");
    return Ok();
}

void SpriteRenderer::shutdown() {
    if (!m_device) return;
    VkDevice dev = m_device->logical_device();
    if (dev != VK_NULL_HANDLE) {
        if (m_pipeline)          vkDestroyPipeline(dev, m_pipeline, nullptr);
        if (m_pipeline_layout)   vkDestroyPipelineLayout(dev, m_pipeline_layout, nullptr);
        if (m_camera_pool)       vkDestroyDescriptorPool(dev, m_camera_pool, nullptr);
        if (m_camera_set_layout) vkDestroyDescriptorSetLayout(dev, m_camera_set_layout, nullptr);
        for (auto& pf : m_frames) {
            pf.ubo.shutdown();
        }
        m_quad_vbo.shutdown();
        m_quad_ibo.shutdown();
    }
    m_pipeline          = VK_NULL_HANDLE;
    m_pipeline_layout   = VK_NULL_HANDLE;
    m_camera_pool       = VK_NULL_HANDLE;
    m_camera_set_layout = VK_NULL_HANDLE;
    m_device            = nullptr;
    m_resources         = nullptr;
    m_pending.clear();
}

Result<void> SpriteRenderer::create_quad_buffers() {
    if (auto r = m_quad_vbo.initialize(*m_device, sizeof(kQuadVertices),
                                       rhi::BufferUsage::Vertex, rhi::BufferMemory::DeviceLocal); !r) return r;
    if (auto r = m_quad_vbo.upload(kQuadVertices, sizeof(kQuadVertices)); !r) return r;

    if (auto r = m_quad_ibo.initialize(*m_device, sizeof(kQuadIndices),
                                       rhi::BufferUsage::Index, rhi::BufferMemory::DeviceLocal); !r) return r;
    if (auto r = m_quad_ibo.upload(kQuadIndices, sizeof(kQuadIndices)); !r) return r;
    return Ok();
}

Result<void> SpriteRenderer::create_camera_resources() {
    VkDevice dev = m_device->logical_device();

    // Set 0, binding 0: camera UBO accessed from vertex stage.
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo li{};
    li.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 1;
    li.pBindings    = &binding;
    if (vkCreateDescriptorSetLayout(dev, &li, nullptr, &m_camera_set_layout) != VK_SUCCESS) {
        return Err{Error{"Camera descriptor set layout failed"}};
    }

    VkDescriptorPoolSize size{};
    size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    size.descriptorCount = kMaxFramesInFlight;
    VkDescriptorPoolCreateInfo pi{};
    pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.maxSets       = kMaxFramesInFlight;
    pi.poolSizeCount = 1;
    pi.pPoolSizes    = &size;
    if (vkCreateDescriptorPool(dev, &pi, nullptr, &m_camera_pool) != VK_SUCCESS) {
        return Err{Error{"Camera descriptor pool failed"}};
    }

    for (u32 i = 0; i < kMaxFramesInFlight; ++i) {
        auto& pf = m_frames[i];
        if (auto r = pf.ubo.initialize(*m_device, sizeof(mat4),
                                       rhi::BufferUsage::Uniform, rhi::BufferMemory::HostVisible); !r) return r;

        VkDescriptorSetAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool     = m_camera_pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts        = &m_camera_set_layout;
        if (vkAllocateDescriptorSets(dev, &ai, &pf.camera_set) != VK_SUCCESS) {
            return Err{Error{"Failed to allocate camera descriptor set"}};
        }

        VkDescriptorBufferInfo bi{};
        bi.buffer = pf.ubo.handle();
        bi.offset = 0;
        bi.range  = sizeof(mat4);

        VkWriteDescriptorSet w{};
        w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet          = pf.camera_set;
        w.dstBinding      = 0;
        w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.descriptorCount = 1;
        w.pBufferInfo     = &bi;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
    }
    return Ok();
}

Result<void> SpriteRenderer::create_pipeline(VkFormat color_format, const std::string& shader_dir) {
    VkDevice dev = m_device->logical_device();

    auto vert = rhi::load_shader_module(*m_device, shader_dir + "/sprite.vert.spv");
    if (!vert) return Err{vert.error()};
    auto frag = rhi::load_shader_module(*m_device, shader_dir + "/sprite.frag.spv");
    if (!frag) {
        vkDestroyShaderModule(dev, vert.value(), nullptr);
        return Err{frag.error()};
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert.value();
    stages[0].pName  = "main";

    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag.value();
    stages[1].pName  = "main";

    // Vertex layout: [vec2 pos][vec2 uv].
    VkVertexInputBindingDescription vbind{};
    vbind.binding   = 0;
    vbind.stride    = sizeof(Vertex);
    vbind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vattr[2]{};
    vattr[0].location = 0;
    vattr[0].binding  = 0;
    vattr[0].format   = VK_FORMAT_R32G32_SFLOAT;
    vattr[0].offset   = offsetof(Vertex, pos);
    vattr[1].location = 1;
    vattr[1].binding  = 0;
    vattr[1].format   = VK_FORMAT_R32G32_SFLOAT;
    vattr[1].offset   = offsetof(Vertex, uv);

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount   = 1;
    vi.pVertexBindingDescriptions      = &vbind;
    vi.vertexAttributeDescriptionCount = 2;
    vi.pVertexAttributeDescriptions    = vattr;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode    = VK_CULL_MODE_NONE;
    rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState att{};
    att.blendEnable         = VK_TRUE;
    att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    att.colorBlendOp        = VK_BLEND_OP_ADD;
    att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    att.alphaBlendOp        = VK_BLEND_OP_ADD;
    att.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments    = &att;

    const VkDynamicState dyn_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dyn_states;

    // Pipeline layout: 2 sets + push constants.
    VkDescriptorSetLayout sets[2] = {
        m_camera_set_layout,
        m_resources->texture_set_layout(),
    };
    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push.offset     = 0;
    push.size       = sizeof(PushBlock);

    VkPipelineLayoutCreateInfo plci{};
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount         = 2;
    plci.pSetLayouts            = sets;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &push;
    if (vkCreatePipelineLayout(dev, &plci, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
        vkDestroyShaderModule(dev, vert.value(), nullptr);
        vkDestroyShaderModule(dev, frag.value(), nullptr);
        return Err{Error{"vkCreatePipelineLayout failed"}};
    }

    // Vulkan 1.3 dynamic rendering: feed the format via VkPipelineRenderingCreateInfo.
    VkPipelineRenderingCreateInfo rendering_info{};
    rendering_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount    = 1;
    rendering_info.pColorAttachmentFormats = &color_format;

    VkGraphicsPipelineCreateInfo gpi{};
    gpi.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpi.pNext               = &rendering_info;
    gpi.stageCount          = 2;
    gpi.pStages             = stages;
    gpi.pVertexInputState   = &vi;
    gpi.pInputAssemblyState = &ia;
    gpi.pViewportState      = &vp;
    gpi.pRasterizationState = &rs;
    gpi.pMultisampleState   = &ms;
    gpi.pColorBlendState    = &cb;
    gpi.pDynamicState       = &dyn;
    gpi.layout              = m_pipeline_layout;
    gpi.subpass             = 0;

    VkResult r = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpi, nullptr, &m_pipeline);
    vkDestroyShaderModule(dev, vert.value(), nullptr);
    vkDestroyShaderModule(dev, frag.value(), nullptr);
    if (r != VK_SUCCESS) {
        return Err{Error{"vkCreateGraphicsPipelines failed"}};
    }
    return Ok();
}

void SpriteRenderer::begin_frame(u32 frame_index, const Camera2D& camera) {
    m_current_frame = frame_index % kMaxFramesInFlight;
    m_pending.clear();

    auto& pf = m_frames[m_current_frame];
    const mat4 vp = camera.view_projection();
    if (void* dst = pf.ubo.map()) {
        std::memcpy(dst, &vp, sizeof(vp));
    }
}

void SpriteRenderer::draw(const Sprite& sprite, const resources::Texture2D& texture) {
    if (texture.descriptor_set == VK_NULL_HANDLE) return;

    // Model matrix: translate -> rotate around origin -> scale by size.
    mat4 m{1.0f};
    m = glm::translate(m, vec3{sprite.position, 0.0f});
    if (sprite.rotation != 0.0f) {
        const vec2 pivot = sprite.size * sprite.origin;
        m = glm::translate(m, vec3{ pivot, 0.0f});
        m = glm::rotate   (m, sprite.rotation, vec3{0.0f, 0.0f, 1.0f});
        m = glm::translate(m, vec3{-pivot, 0.0f});
    }
    m = glm::scale(m, vec3{sprite.size, 1.0f});

    m_pending.push_back({m, sprite.tint, texture.descriptor_set});
}

void SpriteRenderer::flush(VkCommandBuffer cmd) {
    if (m_pending.empty() || m_pipeline == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    const VkBuffer vbo = m_quad_vbo.handle();
    const VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vbo, &zero);
    vkCmdBindIndexBuffer(cmd, m_quad_ibo.handle(), 0, VK_INDEX_TYPE_UINT16);

    // Camera set (0).
    VkDescriptorSet cam_set = m_frames[m_current_frame].camera_set;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
                            0, 1, &cam_set, 0, nullptr);

    VkDescriptorSet last_texture = VK_NULL_HANDLE;
    for (const auto& d : m_pending) {
        if (d.texture_set != last_texture) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
                                    1, 1, &d.texture_set, 0, nullptr);
            last_texture = d.texture_set;
        }
        PushBlock push{d.model, d.tint};
        vkCmdPushConstants(cmd, m_pipeline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(push), &push);
        vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
    }
}

} // namespace gn::graphics
