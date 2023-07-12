#pragma once

#include "material/material.h"
#include "rhi/vulkan_descriptors.h"
#include "rhi/vulkan_rhi.h"

namespace tinygltf {
class Model;
class Node;
}  // namespace tinygltf

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

enum GlobalBindingSlot {
    kGlobalBindingCamera = 0,
    kGlobalBindingEnvironment,

    kGlobalBindingCount
};

struct CamDataSSBO {
    Mat4x4f view{};
    Mat4x4f proj{};
    Mat4x4f proj_view{};
    Vec3f   cam_pos{};
};

struct EnvDataSSBO {
    Vec4f sunlight_color{};
    Vec3f sunlight_dir{};
    float _padding_sunlight_dir{};

    int32_t debug_idx{};
};

enum MeshInstanceBindingSlot {
    kMeshInstanceBinding = 0,

    kMeshInstanceBindingCount
};

struct MeshInstanceSSBO {
    Mat4x4f object_to_world{};
    Mat4x4f world_to_object{};
};

class RenderResource {
public:
    constexpr static int          kMaxVisibleObjects = 100;
    std::vector<RenderObjectDesc> visible_object_descs{};

    struct {
        vk::DescriptorSet   descriptor_set{};
        vk::AllocatedBuffer staging_buffer{};
        vk::AllocatedBuffer buffer{};
        struct {
            void*        begin{};
            CamDataSSBO* cam{};
            EnvDataSSBO* env{};
        } data{};  // Mapped pointers
    } global{};

    struct {
        vk::DescriptorSet   descriptor_set{};
        vk::AllocatedBuffer staging_buffer{};
        vk::AllocatedBuffer buffer{};
        struct {
            void*             begin{};
            MeshInstanceSSBO* cur_instance{};
        } data{};  // Mapped pointers
    } mesh_instances{};

    std::shared_ptr<VulkanRHI> rhi{};

private:
    using ShaderModuleCache =
        std::array<std::unordered_map<std::string, VkShaderModule>,
                   kShaderTypeCount>;
    ShaderModuleCache shaders_{};

    std::unordered_map<std::string, vk::Texture>               textures_{};
    std::unordered_map<std::string, VkSampler>                 samplers_{};
    std::unordered_map<std::string, Mesh>                      meshes_{};
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_{};

    vk::DescriptorAllocator   descriptor_allocator_{};
    vk::DescriptorLayoutCache descriptor_layout_cache_{};

    vk::DestructorQueue dtor_queue_resource_{};

    VkRenderPass default_vk_render_pass_{};
    uint32_t     default_subpass_idx_{};

public:
    RenderResource(std::shared_ptr<VulkanRHI> rhi) : rhi(rhi) {}

    void Init();

    void Finalize();

    void PushDestructor(std::function<void()>&& destructor);

    void ResetMappedPointers();

    std::vector<uint32_t> GlobalSSBODynamicOffsets() const;

    std::vector<uint32_t> MeshInstanceSSBODynamicOffsets() const;

    VkShaderModule GetShaderModule(const std::string& name, ShaderType type);

    vk::Texture* GetTexture(const std::string& name);

    VkSampler GetSampler(const std::string& name);

    Material* GetMaterial(const std::string& name);

    Mesh* GetMesh(const std::string& name);

    VkShaderModule CreateShaderModule(const std::string& name, ShaderType type);

    VkSampler CreateSampler(const std::string& name, VkSamplerCreateInfo* info);

    Material* CreateMaterial(const std::string& name,
                             const std::string& type_name,
                             VkRenderPass       render_pass,
                             uint32_t           subpass_idx);

    Material* CreateMaterial(const std::string& name,
                             const std::string& type_name) {
        return CreateMaterial(name, type_name, default_vk_render_pass_,
                              default_subpass_idx_);
    }

    Mesh* CreateMeshFromObjFile(const std::string& name,
                                const fs::path&    filepath);

    vk::Texture* CreateTexture2D(const std::string&     name,  //
                                 vk::TextureCreateInfo* info,  //
                                 const void*            pixels);

    vk::Texture* CreateTextureCubeMap(const std::string&     name,
                                      vk::TextureCreateInfo* info,
                                      std::array<void*, 6>&  pixels,
                                      uint32_t               mip_levels);

    vk::Texture* CreateTexture2DFromFile(const std::string& name,
                                         const fs::path&    filepath,
                                         bool               is_srgb);

    vk::Texture* CreateTextureHDRFromFile(const std::string& name,
                                          const fs::path&    filepath);

    vk::Texture* CreateTextureCubeMapFromFile(const std::string& name,
                                              const fs::path&    basepath);

    void LoadFromGLTFFile(const fs::path& filepath);

    vk::DescriptorEditor BeginEditDescriptorSet(
        vk::DescriptorSet* descriptor_set) {
        return vk::DescriptorEditor::Begin(
            &descriptor_allocator_, &descriptor_layout_cache_, descriptor_set);
    }

    void SetDefaultRenderPass(VkRenderPass render_pass, uint32_t subpass_idx) {
        default_vk_render_pass_ = render_pass;
        default_subpass_idx_    = subpass_idx;
    }

private:
    void InitGlobalResource();

    void InitMeshInstancesResource();

    void InitDefaultTextures();

    bool LoadVkShaderModule(const std::string& filepath,
                            VkShaderModule*    p_shader_module);

    void UploadMesh(Mesh* mesh);

    void UploadTexture2D(vk::Texture* texture, const void* pixels,
                         VkImageAspectFlags aspect);

    void UploadTextureCubeMap(vk::Texture*          texture,
                              std::array<void*, 6>& pixels,
                              VkImageAspectFlags aspect, uint32_t mip_levels);

    void GLTFLoadTexture(const std::string& name, tinygltf::Model& gltf_model,
                         int idx, bool is_srgb);

    void GLTFLoadMaterials(const std::string& name,
                           tinygltf::Model&   gltf_model);

    void GLTFGetMeshProperties(const tinygltf::Model& gltf_model,
                               const tinygltf::Node&  gltf_node,
                               size_t& vertex_count, size_t& index_count);

    void GLTFLoadMesh(const tinygltf::Model& model, const tinygltf::Node& node,
                      uint32_t node_index, Mesh* mesh);
};

}  // namespace lumi