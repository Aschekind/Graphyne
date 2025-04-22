/**
 * @file vulkan_resource_manager.hpp
 * @brief Vulkan resource management for Graphyne engine
 */
#pragma once

#include "graphyne/core/memory.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace graphyne::vulkan
{

/**
 * @enum ResourceType
 * @brief Types of Vulkan resources managed by the system
 */
enum class ResourceType
{
    Buffer,
    Image,
    Sampler,
    Shader,
    Pipeline,
    DescriptorSet,
    RenderPass,
    Framebuffer,
    CommandPool
};

/**
 * @struct BufferCreateInfo
 * @brief Parameters for creating a Vulkan buffer
 */
struct BufferCreateInfo
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memoryProperties;
    bool hostVisible;
    const void* initialData = nullptr;
};

/**
 * @struct ImageCreateInfo
 * @brief Parameters for creating a Vulkan image
 */
struct ImageCreateInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkFormat format;
    VkImageType imageType = VK_IMAGE_TYPE_2D;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags memoryProperties;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    const void* initialData = nullptr;
};

/**
 * @struct ShaderCreateInfo
 * @brief Parameters for creating a Vulkan shader module
 */
struct ShaderCreateInfo
{
    VkShaderStageFlagBits stage;
    std::string entryPoint = "main";
    std::vector<uint32_t> spirvCode;
};

/**
 * @class VulkanResource
 * @brief Base class for all Vulkan resources
 */
class VulkanResource
{
public:
    virtual ~VulkanResource() = default;

    /**
     * @brief Get the type of this resource
     * @return ResourceType enum value
     */
    virtual ResourceType getType() const = 0;

    /**
     * @brief Get the size of memory allocated for this resource
     * @return Size in bytes
     */
    virtual VkDeviceSize getAllocatedSize() const = 0;
};

/**
 * @class Buffer
 * @brief Represents a Vulkan buffer resource
 */
class Buffer : public VulkanResource
{
public:
    Buffer(VkDevice device, const BufferCreateInfo& createInfo);
    ~Buffer() override;

    ResourceType getType() const override
    {
        return ResourceType::Buffer;
    }
    VkDeviceSize getAllocatedSize() const override
    {
        return m_memorySize;
    }

    /**
     * @brief Map the buffer memory for CPU access
     * @param offset Offset into the buffer in bytes
     * @param size Size to map in bytes (VK_WHOLE_SIZE for entire buffer)
     * @return Pointer to mapped memory
     */
    void* map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    /**
     * @brief Unmap the buffer memory
     */
    void unmap();

    /**
     * @brief Update buffer data
     * @param data Pointer to source data
     * @param size Size of data in bytes
     * @param offset Offset into buffer in bytes
     */
    void update(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    /**
     * @brief Get the Vulkan buffer handle
     * @return VkBuffer handle
     */
    VkBuffer getHandle() const
    {
        return m_buffer;
    }

    /**
     * @brief Check if the buffer is host visible (mappable)
     * @return True if buffer can be mapped
     */
    bool isHostVisible() const
    {
        return m_hostVisible;
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_memorySize = 0;
    bool m_hostVisible = false;
    void* m_mappedData = nullptr;
};

/**
 * @class Image
 * @brief Represents a Vulkan image resource
 */
class Image : public VulkanResource
{
public:
    Image(VkDevice device, const ImageCreateInfo& createInfo);
    ~Image() override;

    ResourceType getType() const override
    {
        return ResourceType::Image;
    }
    VkDeviceSize getAllocatedSize() const override
    {
        return m_memorySize;
    }

    /**
     * @brief Create an image view for this image
     * @param format Format override (uses image format if VK_FORMAT_UNDEFINED)
     * @param aspectFlags Image aspect flags (color, depth, etc.)
     * @return VkImageView handle
     */
    VkImageView createView(VkFormat format = VK_FORMAT_UNDEFINED,
                           VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

    /**
     * @brief Get the Vulkan image handle
     * @return VkImage handle
     */
    VkImage getHandle() const
    {
        return m_image;
    }

    /**
     * @brief Get the image format
     * @return VkFormat of the image
     */
    VkFormat getFormat() const
    {
        return m_format;
    }

    /**
     * @brief Get the image dimensions
     * @return Width, height, and depth of the image
     */
    VkExtent3D getExtent() const
    {
        return m_extent;
    }

    /**
     * @brief Get the number of mip levels
     * @return Mip level count
     */
    uint32_t getMipLevels() const
    {
        return m_mipLevels;
    }

    /**
     * @brief Transition the image layout
     * @param newLayout New layout to transition to
     * @param commandBuffer Command buffer to record transition commands (if null, a temporary one is created)
     * @param srcStageMask Source pipeline stage mask
     * @param dstStageMask Destination pipeline stage mask
     */
    void transitionLayout(VkImageLayout newLayout,
                          VkCommandBuffer commandBuffer = VK_NULL_HANDLE,
                          VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                          VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_memorySize = 0;
    VkFormat m_format;
    VkExtent3D m_extent;
    uint32_t m_mipLevels;
    uint32_t m_arrayLayers;
    VkImageLayout m_currentLayout;
    std::vector<VkImageView> m_createdViews; // Track views to clean up
};

/**
 * @class Shader
 * @brief Represents a Vulkan shader module
 */
class Shader : public VulkanResource
{
public:
    Shader(VkDevice device, const ShaderCreateInfo& createInfo);
    ~Shader() override;

    ResourceType getType() const override
    {
        return ResourceType::Shader;
    }
    VkDeviceSize getAllocatedSize() const override
    {
        return m_codeSize;
    }

    /**
     * @brief Get the shader module handle
     * @return VkShaderModule handle
     */
    VkShaderModule getHandle() const
    {
        return m_shaderModule;
    }

    /**
     * @brief Get the shader stage
     * @return Shader stage flag bits
     */
    VkShaderStageFlagBits getStage() const
    {
        return m_stage;
    }

    /**
     * @brief Get the entry point name
     * @return Entry point function name
     */
    const std::string& getEntryPoint() const
    {
        return m_entryPoint;
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    VkShaderStageFlagBits m_stage;
    std::string m_entryPoint;
    size_t m_codeSize;
};

/**
 * @class VulkanResourceManager
 * @brief Manages Vulkan resources and their memory allocations
 */
class VulkanResourceManager
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the resource manager
     */
    static VulkanResourceManager& getInstance();

    /**
     * @brief Initialize the resource manager
     * @param physicalDevice Vulkan physical device
     * @param device Vulkan logical device
     * @param queueFamilyIndex Graphics queue family index
     * @return True if initialization succeeded
     */
    bool initialize(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueFamilyIndex);

    /**
     * @brief Shutdown the resource manager and free all resources
     */
    void shutdown();

    /**
     * @brief Create a buffer resource
     * @param name Unique identifier for the buffer
     * @param createInfo Buffer creation parameters
     * @return Shared pointer to the created buffer
     */
    std::shared_ptr<Buffer> createBuffer(const std::string& name, const BufferCreateInfo& createInfo);

    /**
     * @brief Create an image resource
     * @param name Unique identifier for the image
     * @param createInfo Image creation parameters
     * @return Shared pointer to the created image
     */
    std::shared_ptr<Image> createImage(const std::string& name, const ImageCreateInfo& createInfo);

    /**
     * @brief Create a shader resource
     * @param name Unique identifier for the shader
     * @param createInfo Shader creation parameters
     * @return Shared pointer to the created shader
     */
    std::shared_ptr<Shader> createShader(const std::string& name, const ShaderCreateInfo& createInfo);

    /**
     * @brief Get a resource by name
     * @param name Resource identifier
     * @return Shared pointer to the resource, or nullptr if not found
     */
    std::shared_ptr<VulkanResource> getResource(const std::string& name);

    /**
     * @brief Get a typed resource by name
     * @tparam T Resource type (Buffer, Image, etc.)
     * @param name Resource identifier
     * @return Shared pointer to the resource, or nullptr if not found or wrong type
     */
    template <typename T>
    std::shared_ptr<T> getResource(const std::string& name)
    {
        auto resource = getResource(name);
        if (!resource)
            return nullptr;
        return std::dynamic_pointer_cast<T>(resource);
    }

    /**
     * @brief Release a specific resource
     * @param name Resource identifier
     * @return True if the resource was found and released
     */
    bool releaseResource(const std::string& name);

    /**
     * @brief Find memory type index for requirements
     * @param typeFilter Memory type bits from memory requirements
     * @param properties Required memory properties
     * @return Index of suitable memory type
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief Print resource usage statistics
     */
    void printStatistics() const;

    /**
     * @brief Begin a single-time command buffer for immediate execution
     * @return Command buffer handle
     */
    VkCommandBuffer beginSingleTimeCommands();

    /**
     * @brief End a single-time command buffer and submit it
     * @param commandBuffer Command buffer to end
     */
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    // Private constructor for singleton
    VulkanResourceManager() = default;
    VulkanResourceManager(const VulkanResourceManager&) = delete;
    VulkanResourceManager& operator=(const VulkanResourceManager&) = delete;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    uint32_t m_queueFamilyIndex = 0;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    bool m_initialized = false;

    // Resource storage by name
    std::unordered_map<std::string, std::shared_ptr<VulkanResource>> m_resources;

    // Resource tracking for statistics
    size_t m_totalBufferMemory = 0;
    size_t m_totalImageMemory = 0;
    size_t m_totalShaderMemory = 0;
    size_t m_resourceCount = 0;

    // Mutex for thread safety
    mutable std::mutex m_mutex;
};

} // namespace graphyne::vulkan
