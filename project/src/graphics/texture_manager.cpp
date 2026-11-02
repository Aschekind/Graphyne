#include "graphics/texture_manager.h"

#include "rhi/device.h"
#include "utils/logger.h"

namespace gn::graphics {

TextureManager::~TextureManager() {
    shutdown();
}

Result<void> TextureManager::initialize(rhi::Device& device, rhi::Sampler::Filter filter) {
    shutdown();
    m_device = &device;
    if (auto r = m_sampler.initialize(device, filter); !r) return r;
    return m_descriptors.initialize(device, kMaxTextures);
}

void TextureManager::shutdown() {
    // Images first, then the pool and sampler they were bound to.
    m_textures.clear();
    m_by_path.clear();
    m_free_sets.clear();
    m_descriptors.shutdown();
    m_sampler.shutdown();
    m_device = nullptr;
}

Result<TextureHandle> TextureManager::create(const u8* rgba, u32 width, u32 height) {
    if (!m_device) return Err{Error{"TextureManager not initialized"}};
    auto texture = std::make_unique<Texture>();
    if (auto r = texture->image.initialize(*m_device, width, height); !r) return Err{r.error()};
    if (auto r = texture->image.upload_rgba8(rgba); !r) return Err{r.error()};

    if (!m_free_sets.empty()) {
        texture->set = m_free_sets.back();
        m_free_sets.pop_back();
    } else {
        auto set = m_descriptors.allocate();
        if (!set) return Err{set.error()};
        texture->set = set.value();
    }
    m_descriptors.update(texture->set, texture->image.view(), m_sampler.handle());
    return m_textures.emplace(std::move(texture));
}

Result<TextureHandle> TextureManager::load(const std::string& path) {
    if (auto it = m_by_path.find(path); it != m_by_path.end()) return it->second;
    auto image = load_image(path);
    if (!image) return Err{image.error()};
    auto texture = create(image.value());
    if (texture) m_by_path.emplace(path, texture.value());
    return texture;
}

void TextureManager::destroy(TextureHandle handle) {
    auto* slot = m_textures.get(handle);
    if (!slot) return;
    m_device->wait_idle();
    m_free_sets.push_back((*slot)->set);
    for (auto it = m_by_path.begin(); it != m_by_path.end();) {
        it = (it->second == handle) ? m_by_path.erase(it) : std::next(it);
    }
    m_textures.erase(handle);
}

Extent2D TextureManager::size(TextureHandle handle) const {
    const auto* slot = m_textures.get(handle);
    return slot ? Extent2D{(*slot)->image.width(), (*slot)->image.height()} : Extent2D{};
}

VkDescriptorSet TextureManager::descriptor(TextureHandle handle) const {
    const auto* slot = m_textures.get(handle);
    return slot ? (*slot)->set : VK_NULL_HANDLE;
}

} // namespace gn::graphics
