#include "vulkan_descriptors.h"

#include <algorithm>

#include "core/hash.h"
#include "vulkan_utils.h"

namespace lumi {
namespace vk {

void DescriptorAllocator::Init(VkDevice device) { device_ = device; }

void DescriptorAllocator::Finalize() {
    for (auto pool : free_pools_) {
        vkDestroyDescriptorPool(device_, pool, nullptr);
    }
    for (auto pool : used_pools_) {
        vkDestroyDescriptorPool(device_, pool, nullptr);
    }
}

void DescriptorAllocator::ResetPools() {
    for (auto pool : used_pools_) {
        vkResetDescriptorPool(device_, pool, 0);
    }

    free_pools_ = used_pools_;
    used_pools_.clear();
    current_pool_ = VK_NULL_HANDLE;
}

bool DescriptorAllocator::Allocate(vk::DescriptorSet* descriptor_set) {
    if (current_pool_ == VK_NULL_HANDLE) {
        current_pool_ = GrabPool();
        used_pools_.emplace_back(current_pool_);
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &descriptor_set->layout;
    allocInfo.descriptorPool     = current_pool_;
    VkResult result =
        vkAllocateDescriptorSets(device_, &allocInfo, &descriptor_set->set);

    switch (result) {
        case VK_SUCCESS:
            // Success
            return true;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            // Reallocate pool
            break;
        default:
            // Error
            return false;
    }

    // Allocate a new pool and retry
    current_pool_ = GrabPool();
    used_pools_.emplace_back(current_pool_);
    allocInfo.descriptorPool = current_pool_;

    result =
        vkAllocateDescriptorSets(device_, &allocInfo, &descriptor_set->set);
    if (result == VK_SUCCESS) {
        return true;
    }
    return false;
}

VkDescriptorPool DescriptorAllocator::GrabPool() {
    if (free_pools_.empty()) {
        return CreatePool();
    } else {
        VkDescriptorPool pool = free_pools_.back();
        free_pools_.pop_back();
        return pool;
    }
}

VkDescriptorPool DescriptorAllocator::CreatePool(
    VkDescriptorPoolCreateFlags flags) {

    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(descriptor_sizes_.size());
    for (auto [type, coeff] : descriptor_sizes_) {
        sizes.push_back({type, uint32_t(coeff * kMaxSets)});
    }

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = flags;
    pool_info.maxSets       = kMaxSets;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes    = sizes.data();

    VkDescriptorPool descriptorPool;
    VK_CHECK(
        vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptorPool));
    return descriptorPool;
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(
    const DescriptorLayoutInfo& other) const {

    if (bindings.size() != other.bindings.size()) {
        return false;
    }
    // Compare each of the bindings is the same.
    // Bindings are sorted so they will match
    for (int i = 0; i < bindings.size(); i++) {
        auto& lhs = bindings[i];
        auto& rhs = other.bindings[i];

        if (lhs.binding != lhs.binding) {
            return false;
        }
        if (lhs.descriptorType != rhs.descriptorType) {
            return false;
        }
        if (lhs.descriptorCount != rhs.descriptorCount) {
            return false;
        }
        if (lhs.stageFlags != rhs.stageFlags) {
            return false;
        }
    }

    return true;
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const {
    size_t result = 0;
    HashCombine(result, bindings.size());

    for (const auto& b : bindings) {
        HashCombine(result, b.binding);
        HashCombine(result, b.descriptorType);
        HashCombine(result, b.descriptorCount);
        HashCombine(result, b.stageFlags);
    }
    return result;
}

void DescriptorLayoutCache::Init(VkDevice device) { device_ = device; }

void DescriptorLayoutCache::Finalize() {
    for (auto [info, layout] : layout_cache_) {
        vkDestroyDescriptorSetLayout(device_, layout, nullptr);
    }
}

VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(
    VkDescriptorSetLayoutCreateInfo* info) {

    DescriptorLayoutInfo layout_info{};
    layout_info.bindings.reserve(info->bindingCount);
    bool    sorted       = true;
    int32_t last_binding = -1;
    for (uint32_t i = 0; i < info->bindingCount; i++) {
        layout_info.bindings.emplace_back(info->pBindings[i]);

        // Check that the bindings are in strict increasing order
        if (sorted &&
            static_cast<int32_t>(info->pBindings[i].binding) > last_binding) {
            last_binding = info->pBindings[i].binding;
        } else {
            sorted = false;
        }
    }

    if (!sorted) {
        std::sort(layout_info.bindings.begin(), layout_info.bindings.end(),
                  [](const VkDescriptorSetLayoutBinding& a,
                     const VkDescriptorSetLayoutBinding& b) {
                      return a.binding < b.binding;
                  });
    }

    auto it = layout_cache_.find(layout_info);
    // Already in cache
    if (it != layout_cache_.end()) {
        return it->second;
    }

    // Create new layout
    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device_, info, nullptr, &layout);
    layout_cache_[layout_info] = layout;
    return layout;
}

DescriptorEditor DescriptorEditor::Begin(DescriptorAllocator*   allocator,
                                         DescriptorLayoutCache* layout_cache,
                                         vk::DescriptorSet* descriptor_set) {

    DescriptorEditor builder;
    builder.allocator_      = allocator;
    builder.cache_          = layout_cache;
    builder.descriptor_set_ = descriptor_set;
    return builder;
}

DescriptorEditor& DescriptorEditor::BindBuffer(
    uint32_t           binding,     //
    VkDescriptorType   type,        //
    VkShaderStageFlags stageFlags,  //
    VkBuffer           buffer,      //
    VkDeviceSize       offset,      //
    VkDeviceSize       range) {

    auto& layout_binding              = bindings_.emplace_back();
    layout_binding.descriptorCount    = 1;
    layout_binding.descriptorType     = type;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags         = stageFlags;
    layout_binding.binding            = binding;

    auto& buffer_info  = buffer_infos_.emplace_back();
    buffer_info.buffer = buffer;
    buffer_info.offset = offset;
    buffer_info.range  = range;

    auto& write           = writes_.emplace_back();
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext           = nullptr;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = &buffer_info;
    write.dstBinding      = binding;

    return *this;
}

DescriptorEditor& DescriptorEditor::BindImage(  //
    uint32_t           binding,                 //
    VkDescriptorType   type,                    //
    VkShaderStageFlags stageFlags,              //
    VkSampler          sampler,                 //
    VkImageView        imageView,               //
    VkImageLayout      imageLayout) {

    auto& layout_binding              = bindings_.emplace_back();
    layout_binding.descriptorCount    = 1;
    layout_binding.descriptorType     = type;
    layout_binding.pImmutableSamplers = nullptr;
    layout_binding.stageFlags         = stageFlags;
    layout_binding.binding            = binding;

    auto& image_info       = image_infos_.emplace_back();
    image_info.sampler     = sampler;
    image_info.imageView   = imageView;
    image_info.imageLayout = imageLayout;

    auto& write           = writes_.emplace_back();
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext           = nullptr;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = &image_info;
    write.dstBinding      = binding;

    return *this;
}

bool DescriptorEditor::Execute(bool update_only) {
    if (!update_only) {
        LOG_ASSERT(!bindings_.empty());

        // Build layout
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = nullptr;
        layoutInfo.pBindings    = bindings_.data();
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings_.size());
        descriptor_set_->layout = cache_->CreateDescriptorLayout(&layoutInfo);

        // Allocate descriptor
        bool success = allocator_->Allocate(descriptor_set_);
        if (!success) {
            return false;
        };
    }

    // Write descriptor
    for (auto& write : writes_) {
        write.dstSet = descriptor_set_->set;
    }
    vkUpdateDescriptorSets(allocator_->device(),
                           static_cast<uint32_t>(writes_.size()),
                           writes_.data(), 0, nullptr);

    return true;
}

}  // namespace vk
}  // namespace lumi