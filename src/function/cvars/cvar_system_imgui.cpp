#include "cvar_system_private.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace lumi {

// definitions of cvars namespace functions
namespace cvars {

void UpdateCachedDescs() {
    CVarSystem& cvar_system = CVarSystem::Instance();

    CVarSystem::ImGuiContext& ctx = cvar_system.imgui_ctx;
    ctx.cached_descs.clear();

    auto FilterCVarDesc = [&ctx](CVarDesc* p_desc) {
        bool is_readonly = p_desc->flags & CVarFlags::kReadOnly;
        bool is_advanced = p_desc->flags & CVarFlags::kAdvanced;

        if (!ctx.show_readonly && is_readonly) return;
        if (!ctx.show_advanced && is_advanced) return;

        if (p_desc->name.find(ctx.search_text) != std::string::npos) {
            ctx.cached_descs.emplace_back(p_desc);
        };
    };

    for (auto& [hash, desc] : cvar_system.table) {
        FilterCVarDesc(&desc);
    }

    std::sort(
        ctx.cached_descs.begin(), ctx.cached_descs.end(),
        [](const CVarDesc* a, const CVarDesc* b) { return a->name < b->name; });
}

void ImGuiRender() {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (!ImGui::TreeNode("Console Variables")) return;

    ImGuiIO& io = ImGui::GetIO();
    CVarSystem::ImGuiContext& ctx = CVarSystem::Instance().imgui_ctx;

    ImGui::Text("Search");
    ImGui::SameLine();
    if (ImGui::InputText("##Search", &ctx.search_text)) {
        // Text edited
        UpdateCachedDescs();
    }
    ImGui::Checkbox("Advanced", &ctx.show_advanced);
    ImGui::Checkbox("Show Read Only", &ctx.show_readonly);
    ImGui::Separator();

    // Show cvars



    ImGui::TreePop();

    //static bool   show_demo_window    = true;
    //static bool   show_another_window = false;
    //static ImVec4 clear_color         = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    //Vec3f* p_clear_color = cvars::GetVec3f("color").ptr();

    //// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    //if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    //// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    //{
    //    static float f       = 0.0f;
    //    static int   counter = 0;
    //    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);

    //    // buffer for fps
    //    char title[32];
    //    sprintf_s(title, "Menu (FPS = %.1f)###title", io.Framerate);
    //    ImGui::Begin(title);

    //    if (ImGui::TreeNode("test1")) {
    //        if (ImGui::TreeNode("test1.1")) {
    //            if (ImGui::TreeNode("test1.1.1")) {
    //                if (ImGui::TreeNode("test1.1.1.1")) {
    //                }
    //            }
    //            if (ImGui::TreeNode("test1.1.2")) {
    //            }
    //        }
    //    }
    //    if (ImGui::TreeNode("test2")) {
    //    }

    //    ImGui::Text(
    //        "This is some useful text.");  // Display some text (you can use a format strings too)
        //ImGui::Checkbox(
        //    "Demo Window",
        //    &show_demo_window);  // Edit bools storing our window open/close state
    //    ImGui::Checkbox("Another Window", &show_another_window);

    //    ImGui::SliderFloat(
    //        "float", &f, 0.0f,
    //        1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
    //    ImGui::ColorEdit3(
    //        "clear color",
    //        (float*)p_clear_color);  // Edit 3 floats representing a color

    //    if (ImGui::Button(
    //            "Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
    //        counter++;
    //    ImGui::SameLine();
    //    ImGui::Text("counter = %d", counter);

    //    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
    //                1000.0f / io.Framerate, io.Framerate);
    //    ImGui::End();
    //}

    //// 3. Show another simple window.
    //if (show_another_window) {
    //    ImGui::Begin(
    //        "Another Window",
    //        &show_another_window);  // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    //    ImGui::Text("Hello from another window!");
    //    if (ImGui::Button("Close Me")) show_another_window = false;
    //    ImGui::End();
    //}
}

}  // namespace cvars

}  // namespace lumi
