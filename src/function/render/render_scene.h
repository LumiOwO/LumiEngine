#pragma once

#include "rhi/vulkan_rhi.h"

namespace lumi {

enum class TextureFormat : int {
    kRGB = 0,
    kRGBA,
    kGray,
};

class RenderScene {
public:
    std::vector<vk::RenderObject> renderables{};

private:
    std::unordered_map<std::string, vk::Material> materials_{};
    std::unordered_map<std::string, vk::Mesh>     meshes_{};
    std::unordered_map<std::string, vk::Texture>  textures_{};

    std::shared_ptr<VulkanRHI> rhi_{};
public:
    RenderScene(std::shared_ptr<VulkanRHI> rhi) : rhi_(rhi) {}

    void LoadScene();

    vk::Material* GetMaterial(const std::string& name);

    vk::Mesh* GetMesh(const std::string& name);

private:
    bool LoadMeshFromObjFile(vk::Mesh& mesh, const fs::path& filepath);

    bool LoadTextureFromFile(vk::Texture& texture, const fs::path& filepath,
                             TextureFormat format);
};

}  // namespace lumi