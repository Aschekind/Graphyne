/**
 * @file resources/resource_manager.h
 * @brief Owns textures (and later buffers/materials/meshes) by handle.
 *
 * The manager:
 *   - Decodes images via stb_image
 *   - Uploads to GPU via rhi::Image
 *   - Allocates a per-texture combined-image-sampler descriptor set
 *   - Deduplicates loads by absolute path (load_texture("a.png") twice
 *     returns the same handle)
 *
 * The manager owns the texture descriptor pool and the descriptor set
 * layout that the SpriteRenderer's pipeline binds at set = 1.
 */
#pragma once

#include "core/handle.h"
#include "core/result.h"
#include "core/slot_map.h"
#include "core/types.h"
#include "resources/texture.h"

#include <string>
#include <string_view>
#include <unordered_map>

#include <vulkan/vulkan.h>

namespace gn::rhi { class Device; }

namespace gn::resources {

class ResourceManager {
public:
    struct Config {
        /// Maximum number of textures that can be live at once.
        u32 max_textures = 256;
    };

    ResourceManager() = default;
    ~ResourceManager();

    ResourceManager(const ResourceManager&)            = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&)                 = delete;
    ResourceManager& operator=(ResourceManager&&)      = delete;

    Result<void> initialize(rhi::Device& device, const Config& cfg = {});
    void shutdown();

    /// Load a texture from disk. Path is resolved against the assets dir.
    /// Subsequent loads of the same absolute path return the same handle.
    Result<TextureHandle> load_texture(std::string_view path);

    /// Create a texture from raw RGBA8 pixels (e.g. procedural data).
    Result<TextureHandle> create_texture_rgba8(u32 width, u32 height, const u8* pixels);

    /// Release a texture. After this, the handle is invalid.
    void release_texture(TextureHandle h);

    const Texture2D* get(TextureHandle h) const { return m_textures.get(h); }
    Texture2D*       get(TextureHandle h)       { return m_textures.get(h); }

    /// Descriptor set layout used for the per-texture descriptor set
    /// (set = 1, binding = 0 in the sprite pipeline).
    VkDescriptorSetLayout texture_set_layout() const { return m_texture_set_layout; }

    /// Number of live textures.
    usize texture_count() const { return m_textures.size(); }

private:
    Result<TextureHandle> build_texture(u32 width, u32 height, const u8* pixels);
    VkDescriptorSet allocate_descriptor_set();
    void write_descriptor_set(VkDescriptorSet set, VkImageView view, VkSampler sampler);

    rhi::Device* m_device = nullptr;

    SlotMap<Texture2D, TextureTag>           m_textures;
    std::unordered_map<std::string, TextureHandle> m_path_cache;

    VkSampler             m_default_sampler     = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_texture_set_layout  = VK_NULL_HANDLE;
    VkDescriptorPool      m_descriptor_pool     = VK_NULL_HANDLE;
};

} // namespace gn::resources
