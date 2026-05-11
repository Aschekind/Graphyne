#include "graphics/renderer.h"

#include "platform/window.h"
#include "rhi/device.h"
#include "rhi/swapchain.h"
#include "utils/logger.h"

#include <limits>

namespace gn::graphics {

Renderer::~Renderer() { shutdown(); }

Result<void> Renderer::initialize(rhi::Device& device,
                                  rhi::Swapchain& swapchain,
                                  platform::Window& window,
                                  const Config& cfg)
{
    m_device    = &device;
    m_swapchain = &swapchain;
    m_window    = &window;
    m_config    = cfg;

    auto r1 = m_command_pool.initialize(device, kMaxFramesInFlight);
    if (!r1) return r1;

    for (auto& s : m_sync) {
        if (!rhi::create_frame_sync(device, s)) {
            return Err{Error{"Failed to create frame sync primitives"}};
        }
    }

    GN_INFO("Renderer initialized ({} frames in flight)", kMaxFramesInFlight);
    return Ok();
}

void Renderer::shutdown() {
    if (m_device) {
        m_device->wait_idle();
        for (auto& s : m_sync) {
            rhi::destroy_frame_sync(*m_device, s);
        }
    }
    m_command_pool.shutdown();
    m_device    = nullptr;
    m_swapchain = nullptr;
    m_window    = nullptr;
}

void Renderer::on_resize() {
    m_needs_recreate = true;
}

Result<void> Renderer::recreate_swapchain() {
    if (!m_device || !m_swapchain || !m_window) return Err{Error{"Renderer not initialized"}};
    return m_swapchain->recreate(m_window->extent(), m_config.vsync);
}

void Renderer::transition_image(VkCommandBuffer cmd, VkImage img,
                                VkImageLayout old_layout, VkImageLayout new_layout,
                                VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                                VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)
{
    VkImageMemoryBarrier2 b{};
    b.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    b.srcStageMask     = src_stage;
    b.srcAccessMask    = src_access;
    b.dstStageMask     = dst_stage;
    b.dstAccessMask    = dst_access;
    b.oldLayout        = old_layout;
    b.newLayout        = new_layout;
    b.image            = img;
    b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.baseMipLevel   = 0;
    b.subresourceRange.levelCount     = 1;
    b.subresourceRange.baseArrayLayer = 0;
    b.subresourceRange.layerCount     = 1;

    VkDependencyInfo dep{};
    dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers    = &b;

    vkCmdPipelineBarrier2(cmd, &dep);
}

Frame* Renderer::begin_frame(f64 time, f32 delta) {
    if (!m_device || !m_swapchain || !m_window) return nullptr;
    if (m_window->minimized()) return nullptr;

    if (m_needs_recreate) {
        auto r = recreate_swapchain();
        if (!r) {
            GN_ERROR("Swapchain recreate failed: {}", r.error().what());
            return nullptr;
        }
        m_needs_recreate = false;
    }

    VkDevice dev   = m_device->logical_device();
    auto&    sync  = m_sync[m_frame_index];

    vkWaitForFences(dev, 1, &sync.in_flight, VK_TRUE, std::numeric_limits<u64>::max());

    bool invalidated = false;
    if (!m_swapchain->acquire_next_image(sync.image_available, m_acquired_image_index, invalidated)) {
        if (invalidated) {
            m_needs_recreate = true;
        }
        return nullptr;
    }

    vkResetFences(dev, 1, &sync.in_flight);

    VkCommandBuffer cmd = m_command_pool.buffer(m_frame_index);
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkImage     image = m_swapchain->images()[m_acquired_image_index];
    VkImageView view  = m_swapchain->image_views()[m_acquired_image_index];

    // UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
    transition_image(cmd, image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,                 0,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

    m_current_frame.frame_index   = m_frame_index;
    m_current_frame.image_index   = m_acquired_image_index;
    m_current_frame.time          = time;
    m_current_frame.delta         = delta;
    m_current_frame.cmd           = cmd;
    m_current_frame.target_view   = view;
    m_current_frame.target_image  = image;
    m_current_frame.target_extent = m_swapchain->extent();
    m_frame_in_progress           = true;

    return &m_current_frame;
}

void Renderer::end_frame() {
    if (!m_frame_in_progress || !m_device || !m_swapchain) return;
    m_frame_in_progress = false;

    VkCommandBuffer cmd  = m_current_frame.cmd;
    VkImage         img  = m_current_frame.target_image;
    VkImageView     view = m_current_frame.target_view;

    // Begin dynamic rendering with the requested clear color.
    VkRenderingAttachmentInfo color{};
    color.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color.imageView   = view;
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color = {{m_current_frame.clear_color.r,
                               m_current_frame.clear_color.g,
                               m_current_frame.clear_color.b,
                               m_current_frame.clear_color.a}};

    VkRenderingInfo ri{};
    ri.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.renderArea.offset    = {0, 0};
    ri.renderArea.extent    = {m_current_frame.target_extent.width,
                               m_current_frame.target_extent.height};
    ri.layerCount           = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments    = &color;

    vkCmdBeginRendering(cmd, &ri);
    // TODO: future SpriteRenderer::flush(cmd) goes here.
    vkCmdEndRendering(cmd);

    // COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
    transition_image(cmd, img,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,          0);

    vkEndCommandBuffer(cmd);

    auto& sync = m_sync[m_frame_index];

    VkSemaphoreSubmitInfo wait{};
    wait.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait.semaphore = sync.image_available;
    wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signal{};
    signal.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal.semaphore = sync.render_finished;
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

    VkCommandBufferSubmitInfo cbi{};
    cbi.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cbi.commandBuffer = cmd;

    VkSubmitInfo2 submit{};
    submit.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.waitSemaphoreInfoCount   = 1;
    submit.pWaitSemaphoreInfos      = &wait;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos    = &signal;
    submit.commandBufferInfoCount   = 1;
    submit.pCommandBufferInfos      = &cbi;

    vkQueueSubmit2(m_device->graphics_queue(), 1, &submit, sync.in_flight);

    if (!m_swapchain->present(m_device->present_queue(), sync.render_finished, m_acquired_image_index)) {
        m_needs_recreate = true;
    }

    m_frame_index = (m_frame_index + 1) % kMaxFramesInFlight;
}

} // namespace gn::graphics
