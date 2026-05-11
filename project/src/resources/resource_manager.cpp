#include "resources/resource_manager.h"

#include "resources/asset_paths.h"
#include "resources/image_loader.h"
#include "rhi/device.h"
#include "utils/logger.h"

namespace gn::resources {

ResourceManager::~ResourceManager() { shutdown(); }

Result<void> ResourceManager::initialize(rhi::Device& device, const Config& cfg) {
    if (m_device) return Ok();
    m_device = &device;

    // Default sampler (linear, clamp-to-edge).
    m_default_sampler = rhi::create_default_sampler(device);
    if (m_default_sampler == VK_NULL_HANDLE) {
        return Err{Error{"Failed to create default sampler"}};
    }

    // Layout: set=1, binding=0, combined image sampler, fragment stage.
    VkDescriptorSetLayoutBinding binding{};
    binding.binding         = 0;
    binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo li{};
    li.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 1;
    li.pBindings    = &binding;
    if (vkCreateDescriptorSetLayout(device.logical_device(), &li, nullptr, &m_texture_set_layout) != VK_SUCCESS) {
        return Err{Error{"Failed to create texture descriptor set layout"}};
    }

    // Pool sized for `max_textures` combined samplers.
    VkDescriptorPoolSize size{};
    size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    size.descriptorCount = cfg.max_textures;

    VkDescriptorPoolCreateInfo pi{};
    pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pi.maxSets       = cfg.max_textures;
    pi.poolSizeCount = 1;
    pi.pPoolSizes    = &size;
    if (vkCreateDescriptorPool(device.logical_device(), &pi, nullptr, &m_descriptor_pool) != VK_SUCCESS) {
        return Err{Error{"Failed to create texture descriptor pool"}};
    }

    GN_INFO("ResourceManager initialized (capacity: {} textures)", cfg.max_textures);
    return Ok();
}

void ResourceManager::shutdown() {
    if (!m_device) return;
    VkDevice dev = m_device->logical_device();
    if (dev != VK_NULL_HANDLE) {
        // Drop all textures (Image owns its VkImage/View/Memory).
        m_textures.clear();
        m_path_cache.clear();

        if (m_descriptor_pool)    vkDestroyDescriptorPool(dev, m_descriptor_pool, nullptr);
        if (m_texture_set_layout) vkDestroyDescriptorSetLayout(dev, m_texture_set_layout, nullptr);
        if (m_default_sampler)    vkDestroySampler(dev, m_default_sampler, nullptr);
    }
    m_descriptor_pool    = VK_NULL_HANDLE;
    m_texture_set_layout = VK_NULL_HANDLE;
    m_default_sampler    = VK_NULL_HANDLE;
    m_device             = nullptr;
}

Result<TextureHandle> ResourceManager::load_texture(std::string_view path) {
    if (!m_device) return Err{Error{"ResourceManager not initialized"}};

    const std::string resolved = resolve_asset(path);

    // Path-cache hit: return same handle (refcounting could go here later).
    if (auto it = m_path_cache.find(resolved); it != m_path_cache.end()) {
        if (m_textures.contains(it->second)) {
            return it->second;
        }
        m_path_cache.erase(it);  // stale entry
    }

    auto decoded = load_image_rgba8(resolved);
    if (!decoded) return Err{decoded.error()};

    auto h = build_texture(decoded.value().width,
                           decoded.value().height,
                           decoded.value().data.data());
    if (!h) return h;

    m_path_cache.emplace(resolved, h.value());
    return h.value();
}

Result<TextureHandle> ResourceManager::create_texture_rgba8(u32 width, u32 height, const u8* pixels) {
    if (!m_device) return Err{Error{"ResourceManager not initialized"}};
    if (!pixels || width == 0 || height == 0) {
        return Err{Error{"create_texture_rgba8: invalid input"}};
    }
    return build_texture(width, height, pixels);
}

Result<TextureHandle> ResourceManager::build_texture(u32 width, u32 height, const u8* pixels) {
    Texture2D tex;
    rhi::ImageDesc desc;
    desc.width  = width;
    desc.height = height;
    desc.format = VK_FORMAT_R8G8B8A8_SRGB;
    desc.usage  = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (auto r = tex.image.initialize(*m_device, desc); !r) return Err{r.error()};
    const usize byte_count = static_cast<usize>(width) * height * 4u;
    if (auto r = tex.image.upload_pixels({pixels, byte_count}); !r) return Err{r.error()};

    tex.sampler = m_default_sampler;
    tex.descriptor_set = allocate_descriptor_set();
    if (tex.descriptor_set == VK_NULL_HANDLE) {
        return Err{Error{"Descriptor pool exhausted"}};
    }
    write_descriptor_set(tex.descriptor_set, tex.image.view(), tex.sampler);

    const TextureHandle h = m_textures.emplace(std::move(tex));
    GN_INFO("Texture created (handle index={} gen={}, {}x{}, total live: {})",
            h.index(), h.generation(), width, height, m_textures.size());
    return h;
}

void ResourceManager::release_texture(TextureHandle h) {
    if (!m_device) return;
    if (Texture2D* t = m_textures.get(h)) {
        if (t->descriptor_set != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_device->logical_device(), m_descriptor_pool, 1, &t->descriptor_set);
            t->descriptor_set = VK_NULL_HANDLE;
        }
        // Sampler is shared; do not destroy it here.
        t->sampler = VK_NULL_HANDLE;
        // tex.image destructor (via Image::shutdown) runs when erase() pops it.
    }
    // Drop the path cache entries pointing at this handle.
    for (auto it = m_path_cache.begin(); it != m_path_cache.end(); ) {
        if (it->second.packed == h.packed) it = m_path_cache.erase(it);
        else ++it;
    }
    m_textures.erase(h);
}

VkDescriptorSet ResourceManager::allocate_descriptor_set() {
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = m_descriptor_pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &m_texture_set_layout;
    VkDescriptorSet set = VK_NULL_HANDLE;
    vkAllocateDescriptorSets(m_device->logical_device(), &ai, &set);
    return set;
}

void ResourceManager::write_descriptor_set(VkDescriptorSet set, VkImageView view, VkSampler sampler) {
    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView   = view;
    info.sampler     = sampler;

    VkWriteDescriptorSet w{};
    w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet          = set;
    w.dstBinding      = 0;
    w.dstArrayElement = 0;
    w.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.descriptorCount = 1;
    w.pImageInfo      = &info;
    vkUpdateDescriptorSets(m_device->logical_device(), 1, &w, 0, nullptr);
}

} // namespace gn::resources
