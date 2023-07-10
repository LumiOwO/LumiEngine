#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "vulkan_types.h"

namespace lumi {
namespace vk {

class DescriptorAllocator {
private:
    constexpr static int kMaxSets = 1000;

    std::vector<std::pair<VkDescriptorType, float>> descriptor_sizes_ = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f},
    };

    VkDescriptorPool              current_pool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorPool> used_pools_{};
    std::vector<VkDescriptorPool> free_pools_{};

    VkDevice device_{};

public:
    void Init(VkDevice device);

    void Finalize();

    void ResetPools();

    bool Allocate(vk::DescriptorSet* descriptor_set);

    VkDevice device() const { return device_; }

private:
    VkDescriptorPool GrabPool();

    VkDescriptorPool CreatePool(VkDescriptorPoolCreateFlags flags = 0);
};

class DescriptorLayoutCache {
public:
    struct DescriptorLayoutInfo {
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        bool operator==(const DescriptorLayoutInfo& other) const;

        size_t hash() const;

        struct Hash {
            std::size_t operator()(const DescriptorLayoutInfo& k) const {
                return k.hash();
            }
        };
    };

private:
    VkDevice device_{};

    using LayoutCache =
        std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout,
                           DescriptorLayoutInfo::Hash>;
    LayoutCache layout_cache_{};

public:
    void Init(VkDevice device);

    void Finalize();

    VkDescriptorSetLayout CreateDescriptorLayout(
        VkDescriptorSetLayoutCreateInfo* info);
};

class DescriptorEditor {
public:
    static DescriptorEditor Begin(DescriptorAllocator*   allocator,
                                  DescriptorLayoutCache* layout_cache,
                                  vk::DescriptorSet*     descriptor_set);

    DescriptorEditor& BindBuffer(uint32_t           binding,     //
                                 VkDescriptorType   type,        //
                                 VkShaderStageFlags stageFlags,  //
                                 VkBuffer           buffer,      //
                                 VkDeviceSize       offset,      //
                                 VkDeviceSize       range);

    DescriptorEditor& BindImage(uint32_t           binding,     //
                                VkDescriptorType   type,        //
                                VkShaderStageFlags stageFlags,  //
                                VkSampler          sampler,     //
                                VkImageView        imageView,   //
                                VkImageLayout      imageLayout);

    bool Execute(bool update_only = false);

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings_{};
    std::vector<VkWriteDescriptorSet>         writes_{};
    std::list<VkDescriptorBufferInfo>         buffer_infos_{};
    std::list<VkDescriptorImageInfo>          image_infos_{};

    DescriptorAllocator*   allocator_      = nullptr;
    DescriptorLayoutCache* cache_          = nullptr;
    vk::DescriptorSet*     descriptor_set_ = nullptr;
};

}  // namespace vk
}  // namespace lumi
