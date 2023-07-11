#include "imgui_subpass.h"

#include "function/cvars/cvar_system.h"
#include "function/render/pipeline/pass/render_pass.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"

namespace lumi {

void ImGuiSubpass::Init(uint32_t subpass_idx) {
    render_pass_->rhi->CreateImGuiContext(render_pass_->vk_render_pass(),
                                          subpass_idx);
}

void ImGuiSubpass::DestroyImGuiContext() {
    render_pass_->rhi->DestroyImGuiContext();
}

void ImGuiSubpass::CmdRender(VkCommandBuffer cmd) {
    ImGui_ImplVulkan_NewFrame();
    render_pass_->rhi->ImGuiWindowNewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    //ImGui::ShowDemoWindow();
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(main_viewport->WorkPos.x + 75, main_viewport->WorkPos.y + 50),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(450, 750), ImGuiCond_FirstUseEver);

#pragma region Main menu
    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);

    char title[32];
    sprintf_s(title, "Menu (FPS = %.1f)###menu", io.Framerate);
    ImGui::Begin(title);

    cvars::ImGuiRender();

    ImGui::End();
#pragma endregion

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

}  // namespace lumi