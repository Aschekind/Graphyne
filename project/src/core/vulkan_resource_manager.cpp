/**
 * @file vulkan_resource_manager.cpp
 * @brief Implementation of Vulkan resource management for Graphyne engine
 */
#include "graphyne/core/vulkan_resource_manager.hpp"
#include "graphyne/utils/logger.hpp"
#include <algorithm>
#include <cassert>

namespace graphyne::vulkan
{

// Buffer implementation
Buffer::Buffer(VkDevice device, const BufferCreateInfo& createInfo)
    : m_device(device),
      m_memorySize(createInfo.size),
      m_hostVisible(createInfo.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
{
    // Create buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = createInfo.size;
    bufferInfo.usage = createInfo.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
    {
        GN_ERROR("Failed to create Vulkan buffer");
        return;
    }

    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanResourceManager::getInstance().findMemoryType(memRequirements.memoryTypeBits,
                                                                                    createInfo.memoryProperties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
    {
        GN_ERROR("Failed to allocate buffer memory");
        vkDestroyBuffer(device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        return;
    }

    // Bind memory to buffer
    vkBindBufferMemory(device, m_buffer, m_memory, 0);

    // If initial data is provided, upload it
    if (createInfo.initialData && m_hostVisible)
    {
        void* mappedData = map();
        if (mappedData)
        {
            memcpy(mappedData, createInfo.initialData, static_cast<size_t>(createInfo.size));
            unmap();
        }
    }
}

Buffer::~Buffer()
{
    if (m_mappedData)
    {
        unmap();
    }

    if (m_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_memory, nullptr);
    }

    if (m_buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_buffer, nullptr);
    }
}

void* Buffer::map(VkDeviceSize offset, VkDeviceSize size)
{
    if (!m_hostVisible)
    {
        GN_ERROR("Attempting to map a non-host-visible buffer");
        return nullptr;
    }

    if (m_mappedData)
    {
        GN_WARNING("Buffer already mapped, returning existing mapping");
        return m_mappedData;
    }

    if (vkMapMemory(m_device, m_memory, offset, size, 0, &m_mappedData) != VK_SUCCESS)
    {
        GN_ERROR("Failed to map buffer memory");
        return nullptr;
    }

    return m_mappedData;
}

void Buffer::unmap()
{
    if (m_mappedData)
    {
        vkUnmapMemory(m_device, m_memory);
        m_mappedData = nullptr;
    }
}

void Buffer::update(const void* data, VkDeviceSize size, VkDeviceSize offset)
{
    if (!m_hostVisible)
    {
        GN_ERROR("Cannot update a non-host-visible buffer directly");
        return;
    }

    void* mappedData = map(offset, size);
    if (mappedData)
    {
        memcpy(mappedData, data, static_cast<size_t>(size));

        // If memory is coherent, no need to flush
        if (!(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            VkMappedMemoryRange range{};
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.memory = m_memory;
            range.offset = offset;
            range.size = size;
            vkFlushMappedMemoryRanges(m_device, 1, &range);
        }

        unmap();
    }
}

// Image implementation
Image::Image(VkDevice device, const ImageCreateInfo& createInfo)
    : m_device(device),
      m_format(createInfo.format),
      m_mipLevels(createInfo.mipLevels),
      m_arrayLayers(createInfo.arrayLayers),
      m_currentLayout(createInfo.initialLayout)
{
    // Set up image dimensions
    m_extent.width = createInfo.width;
    m_extent.height = createInfo.height;
    m_extent.depth = createInfo.depth;

    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = createInfo.imageType;
    imageInfo.format = createInfo.format;
    imageInfo.extent = m_extent;
    imageInfo.mipLevels = createInfo.mipLevels;
    imageInfo.arrayLayers = createInfo.arrayLayers;
    imageInfo.samples = createInfo.samples;
    imageInfo.tiling = createInfo.tiling;
    imageInfo.usage = createInfo.usage;
    imageInfo.initialLayout = createInfo.initialLayout;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
    {
        GN_ERROR("Failed to create Vulkan image");
        return;
    }

    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_image, &memRequirements);

    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanResourceManager::getInstance().findMemoryType(memRequirements.memoryTypeBits,
                                                                                    createInfo.memoryProperties);

    m_memorySize = memRequirements.size;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
    {
        GN_ERROR("Failed to allocate image memory");
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return;
    }

    // Bind memory to image
    vkBindImageMemory(device, m_image, m_memory, 0);

    // If initial data is provided, we need to handle uploading it
    // This would involve staging buffer, command buffers, etc.
    // Omitted for brevity, but would be implemented here
}

Image::~Image()
{
    // Clean up all created views
    for (auto view : m_createdViews)
    {
        if (view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_device, view, nullptr);
        }
    }

    if (m_memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_memory, nullptr);
    }

    if (m_image != VK_NULL_HANDLE)
    {
        vkDestroyImage(m_device, m_image, nullptr);
    }
}

VkImageView Image::createView(VkFormat format, VkImageAspectFlags aspectFlags)
{
    if (format == VK_FORMAT_UNDEFINED)
    {
        format = m_format;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Adjust for 3D/cube/array textures
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_arrayLayers;

    VkImageView imageView;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        GN_ERROR("Failed to create image view");
        return VK_NULL_HANDLE;
    }

    // Store for cleanup
    m_createdViews.push_back(imageView);

    return imageView;
}

void Image::transitionLayout(VkImageLayout newLayout,
                             VkCommandBuffer commandBuffer,
                             VkPipelineStageFlags srcStageMask,
                             VkPipelineStageFlags dstStageMask)
{
    if (m_currentLayout == newLayout)
    {
        return; // No transition needed
    }

    // Check if we need to create a command buffer
    bool ownCommandBuffer = false;
    if (commandBuffer == VK_NULL_HANDLE)
    {
        commandBuffer = VulkanResourceManager::getInstance().beginSingleTimeCommands();
        ownCommandBuffer = true;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;

    // Set aspect mask based on image format and new layout
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        // If format has stencil component
        if (m_format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_arrayLayers;

    // Set access masks and pipeline stages based on layouts
    VkAccessFlags srcAccessMask = 0;
    VkAccessFlags dstAccessMask = 0;

    // Source access mask
    if (m_currentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    {
        srcAccessMask = 0;
    }
    else if (m_currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (m_currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    // Destination access mask
    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update current layout
    m_currentLayout = newLayout;

    // If we created our own command buffer, submit it
    if (ownCommandBuffer)
    {
        VulkanResourceManager::getInstance().endSingleTimeCommands(commandBuffer);
    }
}

// Shader implementation
Shader::Shader(VkDevice device, const ShaderCreateInfo& createInfo)
    : m_device(device),
      m_stage(createInfo.stage),
      m_entryPoint(createInfo.entryPoint),
      m_codeSize(createInfo.spirvCode.size() * sizeof(uint32_t))
{
    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = m_codeSize;
    moduleInfo.pCode = createInfo.spirvCode.data();

    if (vkCreateShaderModule(device, &moduleInfo, nullptr, &m_shaderModule) != VK_SUCCESS)
    {
        GN_ERROR("Failed to create shader module");
    }
}

Shader::~Shader()
{
    if (m_shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    }
}

// VulkanResourceManager implementation
VulkanResourceManager& VulkanResourceManager::getInstance()
{
    static VulkanResourceManager instance;
    return instance;
}

bool VulkanResourceManager::initialize(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueFamilyIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized)
    {
        GN_WARNING("Vulkan resource manager already initialized");
        return true;
    }

    m_physicalDevice = physicalDevice;
    m_device = device;
    m_queueFamilyIndex = queueFamilyIndex;

    // Get the graphics queue handle
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &m_graphicsQueue);

    // Create command pool for resource operations
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        GN_ERROR("Failed to create command pool for resource manager");
        return false;
    }

    m_initialized = true;
    GN_INFO("Vulkan resource manager initialized successfully");
    return true;
}

void VulkanResourceManager::shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized)
    {
        return;
    }

    // Print statistics before cleanup
    printStatistics();

    // Release all resources
    m_resources.clear();

    // Destroy command pool
    if (m_commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    // Reset counters and state
    m_totalBufferMemory = 0;
    m_totalImageMemory = 0;
    m_totalShaderMemory = 0;
    m_resourceCount = 0;
    m_initialized = false;

    GN_INFO("Vulkan resource manager shutdown complete");
}

std::shared_ptr<Buffer> VulkanResourceManager::createBuffer(const std::string& name, const BufferCreateInfo& createInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized)
    {
        GN_ERROR("Vulkan resource manager not initialized");
        return nullptr;
    }

    // Check if resource with this name already exists
    if (m_resources.find(name) != m_resources.end())
    {
        GN_WARNING("Resource with name '{}' already exists, returning existing resource", name);
        auto existingResource = std::dynamic_pointer_cast<Buffer>(m_resources[name]);
        if (existingResource)
        {
            return existingResource;
        }
        else
        {
            GN_ERROR("Resource with name '{}' exists but is not a buffer", name);
            return nullptr;
        }
    }

    // Create new buffer
    auto buffer = std::make_shared<Buffer>(m_device, createInfo);
    if (!buffer || buffer->getHandle() == VK_NULL_HANDLE)
    {
        GN_ERROR("Failed to create buffer '{}'", name);
        return nullptr;
    }

    // Store the resource
    m_resources[name] = buffer;
    m_resourceCount++;
    m_totalBufferMemory += buffer->getAllocatedSize();

    GN_INFO("Created buffer '{}' with size {}", name, buffer->getAllocatedSize());
    return buffer;
}

std::shared_ptr<Image> VulkanResourceManager::createImage(const std::string& name, const ImageCreateInfo& createInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized)
    {
        GN_ERROR("Vulkan resource manager not initialized");
        return nullptr;
    }

    // Check if resource with this name already exists
    if (m_resources.find(name) != m_resources.end())
    {
        GN_WARNING("Resource with name '{}' already exists, returning existing resource", name);
        auto existingResource = std::dynamic_pointer_cast<Image>(m_resources[name]);
        if (existingResource)
        {
            return existingResource;
        }
        else
        {
            GN_ERROR("Resource with name '{}' exists but is not an image", name);
            return nullptr;
        }
    }

    // Create new image
    auto image = std::make_shared<Image>(m_device, createInfo);
    if (!image || image->getHandle() == VK_NULL_HANDLE)
    {
        GN_ERROR("Failed to create image '{}'", name);
        return nullptr;
    }

    // Store the resource
    m_resources[name] = image;
    m_resourceCount++;
    m_totalImageMemory += image->getAllocatedSize();

    GN_INFO("Created image '{}' with size {}", name, image->getAllocatedSize());
    return image;
}

std::shared_ptr<Shader> VulkanResourceManager::createShader(const std::string& name, const ShaderCreateInfo& createInfo)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized)
    {
        GN_ERROR("Vulkan resource manager not initialized");
        return nullptr;
    }

    // Check if resource with this name already exists
    if (m_resources.find(name) != m_resources.end())
    {
        GN_WARNING("Resource with name '{}' already exists, returning existing resource", name);
        auto existingResource = std::dynamic_pointer_cast<Shader>(m_resources[name]);
        if (existingResource)
        {
            return existingResource;
        }
        else
        {
            GN_ERROR("Resource with name '{}' exists but is not a shader", name);
            return nullptr;
        }
    }

    // Create new shader
    auto shader = std::make_shared<Shader>(m_device, createInfo);
    if (!shader || shader->getHandle() == VK_NULL_HANDLE)
    {
        GN_ERROR("Failed to create shader '{}'", name);
        return nullptr;
    }

    // Store the resource
    m_resources[name] = shader;
    m_resourceCount++;
    m_totalShaderMemory += shader->getAllocatedSize();

    GN_INFO("Created shader '{}' with size {}", name, shader->getAllocatedSize());
    return shader;
}

std::shared_ptr<VulkanResource> VulkanResourceManager::getResource(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_resources.find(name);
    if (it == m_resources.end())
    {
        return nullptr;
    }

    return it->second;
}

bool VulkanResourceManager::releaseResource(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_resources.find(name);
    if (it == m_resources.end())
    {
        return false;
    }

    // Update statistics based on resource type
    auto resource = it->second;
    if (resource)
    {
        switch (resource->getType())
        {
            case ResourceType::Buffer:
                m_totalBufferMemory -= resource->getAllocatedSize();
                break;
            case ResourceType::Image:
                m_totalImageMemory -= resource->getAllocatedSize();
                break;
            case ResourceType::Shader:
                m_totalShaderMemory -= resource->getAllocatedSize();
                break;
            default:
                break;
        }
    }

    m_resources.erase(it);
    m_resourceCount--;

    GN_INFO("Released resource '{}'", name);
    return true;
}

uint32_t VulkanResourceManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    GN_ERROR("Failed to find suitable memory type");
    return 0;
}

VkCommandBuffer VulkanResourceManager::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanResourceManager::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanResourceManager::printStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    GN_INFO("Vulkan Resource Manager Statistics:");
    GN_INFO("  Total Resources: {}", m_resourceCount);
    GN_INFO("  Total Buffer Memory: {} bytes", m_totalBufferMemory);
    GN_INFO("  Total Image Memory: {} bytes", m_totalImageMemory);
    GN_INFO("  Total Shader Memory: {} bytes", m_totalShaderMemory);
    GN_INFO("  Total Managed Memory: {} bytes", m_totalBufferMemory + m_totalImageMemory + m_totalShaderMemory);

    // Print individual resource details
    GN_INFO("Resource Breakdown:");
    size_t bufferCount = 0, imageCount = 0, shaderCount = 0, otherCount = 0;

    for (const auto& [name, resource] : m_resources)
    {
        if (!resource)
            continue;

        switch (resource->getType())
        {
            case ResourceType::Buffer:
                bufferCount++;
                break;
            case ResourceType::Image:
                imageCount++;
                break;
            case ResourceType::Shader:
                shaderCount++;
                break;
            default:
                otherCount++;
                break;
        }
    }

    GN_INFO("  Buffers: {}", bufferCount);
    GN_INFO("  Images: {}", imageCount);
    GN_INFO("  Shaders: {}", shaderCount);
    GN_INFO("  Other: {}", otherCount);
}

} // namespace graphyne::vulkan
