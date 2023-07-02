#include "cvar_system_private.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "core/scope_guard.h"

namespace lumi {

// definitions of cvars namespace functions
namespace cvars {

void CacheCVar(CVarSystem::ImGuiCVarTreeNode* root, CVarDesc* desc) {
    const std::string& name      = desc->name;
    size_t             start_pos = 0;

    // nesting
    auto p = root;
    do {
        size_t end_pos = name.find('.', start_pos);
        if (end_pos == std::string::npos) break;

        const std::string& level_name =
            name.substr(start_pos, end_pos - start_pos);
        p         = &p->children[level_name];
        p->name   = level_name;
        start_pos = end_pos + 1;
    } while (true);

    p->descs.insert(desc);
}

void UpdateCachedCVars() {
    CVarSystem& cvar_system = CVarSystem::Instance();

    CVarSystem::ImGuiContext& ctx = cvar_system.imgui_ctx;
    ctx.cached_cvars_root         = {};

    for (auto& [hash, desc] : cvar_system.table) {
        bool is_readonly = desc.flags & CVarFlags::kReadOnly;
        bool is_advanced = desc.flags & CVarFlags::kAdvanced;

        if (!ctx.show_readonly && is_readonly) continue;
        if (!ctx.show_advanced && is_advanced) continue;

        if (ctx.search_text.empty() ||
            desc.name.find(ctx.search_text) != std::string::npos) {
            CacheCVar(&ctx.cached_cvars_root, &desc);
        };
    };
}

void ImGuiShowCVarsDesc(CVarDesc* desc) {
    ImGui::PushID(desc);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -0.01f);
    if (desc->flags & CVarFlags::kReadOnly) {
        ImGui::BeginDisabled();
    }

    ScopeGuard guard = [desc]() {
        if (desc->flags & CVarFlags::kReadOnly) {
            ImGui::EndDisabled();
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
    };
     
    bool is_unit = (desc->flags & CVarFlags::kIsUnit) ||
                   (desc->flags & CVarFlags::kIsColor);
    float v_min   = desc->min;
    float v_max   = desc->max;
    float v_speed = (v_min != v_max && v_max - v_min <= 2.f) ? 0.001f : .125f;

    CVarSystem& cvar_system = CVarSystem::Instance();
    if (desc->type == CVarType::kBool) {
        ImGui::Checkbox("", cvar_system.GetPtr<BoolType>(desc->index_));
    } else if (desc->type == CVarType::kString) {
        ImGui::InputText("", cvar_system.GetPtr<StringType>(desc->index_));
    } else if (desc->type == CVarType::kInt) {
        ImGui::DragInt("", cvar_system.GetPtr<IntType>(desc->index_), v_speed,
                       (int)v_min, (int)v_max, "%d",
                       ImGuiSliderFlags_AlwaysClamp);
    } else if (desc->type == CVarType::kFloat) {
        ImGui::DragFloat("", cvar_system.GetPtr<FloatType>(desc->index_),
                         v_speed, v_min, v_max, "%.3f",
                         ImGuiSliderFlags_AlwaysClamp);
    } else if (desc->type == CVarType::kVec2f) {
        ImGui::DragFloat2(
            "", (float*)cvar_system.GetPtr<Vec2fType>(desc->index_), v_speed,
            v_min, v_max, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    } else if (desc->type == CVarType::kVec3f) {
        if (desc->flags & CVarFlags::kIsColor) {
            ImGui::ColorPicker3(
                "", (float*)cvar_system.GetPtr<Vec3fType>(desc->index_),
                ImGuiColorEditFlags_DefaultOptions_ |
                    ImGuiColorEditFlags_NoSidePreview);
        } else {
            ImGui::DragFloat3(
                "", (float*)cvar_system.GetPtr<Vec3fType>(desc->index_),
                v_speed, v_min, v_max, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        }
    } else if (desc->type == CVarType::kVec4f) {
        if (desc->flags & CVarFlags::kIsColor) {
            ImGui::ColorPicker4(
                "", (float*)cvar_system.GetPtr<Vec4fType>(desc->index_),
                ImGuiColorEditFlags_DefaultOptions_ |
                    ImGuiColorEditFlags_NoSidePreview);
        } else {
            ImGui::DragFloat4(
                "", (float*)cvar_system.GetPtr<Vec4fType>(desc->index_),
                v_speed, v_min, v_max, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        }
    }
}

void ShowCVarsInCurrentNode(CVarSystem::ImGuiCVarTreeNode* node) {
    if (node->name.empty()) {
        // root node
        if (node->descs.empty()) {
            return;
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (!ImGui::TreeNode("< Uncategorized >")) {
            return;
        }
    }
    if (!ImGui::BeginTable("table", 2, ImGuiTableFlags_SizingStretchProp)) {
        return;
    }
    ScopeGuard guard = [node]() {
        ImGui::EndTable();
        if (node->name.empty()) {
            ImGui::TreePop();
        }
    };

    for (auto& desc : node->descs) {
        std::string_view full_name = desc->name;
        size_t           start_pos = full_name.rfind('.');
        start_pos = start_pos == std::string::npos ? 0 : start_pos + 1;

        std::string_view last_name =
            full_name.substr(start_pos, full_name.size());
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text(last_name.data());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(desc->description.c_str());
        }

        ImGui::TableNextColumn();
        ImGuiShowCVarsDesc(desc);
    }
}

void ShowCachedCVars(CVarSystem::ImGuiCVarTreeNode* node) {
    ImGui::PushID(node);
    ScopeGuard guard = []() { ImGui::PopID(); };

    ShowCVarsInCurrentNode(node);

    for (auto& [name, child] : node->children) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (!ImGui::TreeNode(name.c_str())) continue;

        ShowCachedCVars(&child);

        ImGui::TreePop();
    }

}

void ImGuiRender() {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("Console Variables")) return;

    CVarSystem::ImGuiContext& ctx = CVarSystem::Instance().imgui_ctx;
    if (!ctx.inited) {
        UpdateCachedCVars();
        ctx.inited = true;
    }

    ImGui::Text("Search");
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetFontSize() * -0.01f);
    if (ImGui::InputText("##Search", &ctx.search_text)) {
        // Text edited
        UpdateCachedCVars();
    }
    ImGui::PopItemWidth();

    if (ImGui::Checkbox("Show Read Only", &ctx.show_readonly)) {
        UpdateCachedCVars();
    }
    if (ImGui::Checkbox("Show Advanced", &ctx.show_advanced)) {
        UpdateCachedCVars();
    }
    
    ImGui::Spacing();
    ImGui::Separator();

    // Show cvars
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {30, 4});
    ShowCachedCVars(&ctx.cached_cvars_root);
    ImGui::PopStyleVar();
}

}  // namespace cvars

}  // namespace lumi
