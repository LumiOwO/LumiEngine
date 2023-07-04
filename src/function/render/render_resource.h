#pragma once

#include "material/material.h"
#include "rhi/vulkan_descriptors.h"
#include "rhi/vulkan_rhi.h"

namespace lumi {

enum ShaderType {
    kShaderTypeVertex = 0,
    kShaderTypeFragment,
    kShaderTypeCompute,

    kShaderTypeCount
};

struct Mesh {
    using IndexType                           = uint32_t;
    constexpr static VkIndexType kVkIndexType = VK_INDEX_TYPE_UINT32;

    std::vector<vk::Vertex> vertices{};
    std::vector<IndexType>  indices{};
    vk::AllocatedBuffer     vertex_buffer{};
    vk::AllocatedBuffer     index_buffer{};
};

struct RenderObject {
    std::string mesh_name     = "";
    std::string material_name = "";
    Vec3f       position      = Vec3f::kZero;
    Quaternion  rotation      = Quaternion::kIdentity;
    Vec3f       scale         = Vec3f::kUnitScale;
};

struct RenderObjectDesc {
    RenderObject* object   = nullptr;
    Mesh*         mesh     = nullptr;
    Material*     material = nullptr;
};

struct PerFrameBufferObject {
    Mat4x4f view{};
    Mat4x4f proj{};
    Mat4x4f proj_view{};

    Vec4f fog_color{};      // w is for exponent
    Vec4f fog_distances{};  // x for min, y for max, zw unused.
    Vec4f ambient_color{};
    Vec4f sunlight_direction{};  // w for sun power
    Vec4f sunlight_color{};
};

struct PerVisibleBufferObject {
    Mat4x4f model_matrix{};
};

class RenderResource {
public:
    constexpr static int          kMaxVisibleObjects = 100;
    std::vector<RenderObjectDesc> visible_object_descs{};

    struct {
        vk::DescriptorSet     descriptor_set{};
        vk::AllocatedBuffer   staging_buffer{};
        vk::AllocatedBuffer   buffer{};
        void*                 data{};    // Mapped pointer to staging buffer
        PerFrameBufferObject* object{};  // Pointers to current buffer object
    } per_frame = {};

    struct {
        vk::DescriptorSet       descriptor_set{};
        vk::AllocatedBuffer     staging_buffer{};
        vk::AllocatedBuffer     buffer{};
        void*                   data{};    // Mapped pointer to staging buffer
        PerVisibleBufferObject* object{};  // Pointers to current buffer object
    } per_visible = {};

    VkSampler sampler_nearest{};
    VkSampler sampler_linear{};

    std::shared_ptr<VulkanRHI> rhi{};

private:
    using ShaderModuleCache =
        std::array<std::unordered_map<std::string, VkShaderModule>,
                   kShaderTypeCount>;
    ShaderModuleCache shaders_{};

    std::unordered_map<std::string, vk::Texture2D>             textures_{};
    std::unordered_map<std::string, Mesh>                      meshes_{};
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_{};

    vk::DescriptorAllocator   descriptor_allocator_{};
    vk::DescriptorLayoutCache descriptor_layout_cache_{};

    vk::DestructorQueue dtor_queue_resource_{};

public:
    RenderResource(std::shared_ptr<VulkanRHI> rhi) : rhi(rhi) {}

    void Init();

    void Finalize();

    void PushDestructor(std::function<void()>&& destructor);

    void ResetMappedPointers();

    uint32_t GetPerFrameDynamicOffset() const;

    uint32_t GetPerVisibleDynamicOffset() const;

    VkShaderModule GetShaderModule(const std::string& name, ShaderType type);

    vk::Texture2D* GetTexture2D(const std::string& name);

    Material* GetMaterial(const std::string& name);

    Mesh* GetMesh(const std::string& name);

    VkShaderModule CreateShaderModule(const std::string& name, ShaderType type);

    Material* CreateMaterial(const std::string& name,
                             const std::string& type_name,
                             VkRenderPass       render_pass = VK_NULL_HANDLE);

    Mesh* CreateMeshFromObjFile(const std::string& name,
                                const fs::path&    filepath);

    vk::Texture2D* CreateTexture2D(const std::string&       name,  //
                                   vk::Texture2DCreateInfo* info,  //
                                   const void*              pixels);

    vk::Texture2D* CreateTexture2DFromFile(const std::string& name,
                                           const fs::path&    filepath);

    vk::DescriptorEditor EditDescriptorSet(vk::DescriptorSet* descriptor_set) {
        return vk::DescriptorEditor::Begin(
            &descriptor_allocator_, &descriptor_layout_cache_, descriptor_set);
    }

private:
    bool LoadVkShaderModule(const std::string& filepath,
                            VkShaderModule*    p_shader_module);

    void UploadMesh(Mesh* mesh);

    void UploadTexture2D(vk::Texture2D* texture, const void* pixels);
};

}  // namespace lumi