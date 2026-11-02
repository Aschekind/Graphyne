/**
 * @file graphics/texture_manager.h
 * @brief Owns GPU textures (image + descriptor set) addressed by TextureHandle.
 */
#pragma once

#include "core/handle.h"
#include "core/result.h"
#include "core/slot_map.h"
#include "graphics/image_loader.h"
#include "rhi/image.h"
#include "rhi/pipeline.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gn::rhi {
class Device;
}

namespace gn::graphics {

/// Textures are sRGB RGBA8. One sampler is shared by all of them. Not thread-safe.
/// Holds a pointer to the Device: shut it down before the Device.
class TextureManager {
public:
    static constexpr u32 kMaxTextures = 256;

    TextureManager() = default;
    ~TextureManager();

    TextureManager(const TextureManager&)            = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&)                 = delete;
    TextureManager& operator=(TextureManager&&)      = delete;

    Result<void> initialize(rhi::Device& device, rhi::Sampler::Filter filter = rhi::Sampler::Filter::Nearest);
    void         shutdown();

    /// `rgba` is width*height*4 bytes.
    Result<TextureHandle> create(const u8* rgba, u32 width, u32 height);
    Result<TextureHandle> create(const ImageData& image) {
        return create(image.rgba.data(), image.width, image.height);
    }
    /// Decodes the file once; loading the same path again returns the same handle.
    Result<TextureHandle> load(const std::string& path);
    /// Waits for the GPU to be idle, so call it between frames, not per frame.
    void destroy(TextureHandle handle);

    bool     valid(TextureHandle handle) const { return m_textures.contains(handle); }
    Extent2D size(TextureHandle handle) const;

    /// VK_NULL_HANDLE for an unknown handle.
    VkDescriptorSet       descriptor(TextureHandle handle) const;
    VkDescriptorSetLayout set_layout() const { return m_descriptors.layout(); }

private:
    struct Texture {
        rhi::Image      image;
        VkDescriptorSet set = VK_NULL_HANDLE;
    };

    rhi::Device*                                   m_device = nullptr;
    rhi::Sampler                                   m_sampler;
    rhi::SamplerDescriptors                        m_descriptors;
    std::vector<VkDescriptorSet>                   m_free_sets; // sets of destroyed textures; the pool can't free
    SlotMap<std::unique_ptr<Texture>, TextureTag>  m_textures;  // Image can't move, so box it
    std::unordered_map<std::string, TextureHandle> m_by_path;
};

} // namespace gn::graphics
