#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"
#include "vulkan_rhi.h"

#include "function/cvars/cvar_system.h"

namespace lumi {

void ImGuiSetStyle() {
    // Setup Dear ImGui context
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    ImGuiStyle& style    = ImGui::GetStyle();
    //LOG_DEBUG("{}, {}", style.ItemInnerSpacing.x, style.DisabledAlpha);
    style.FrameRounding    = 5.0f;
    style.WindowRounding   = 7.0f;
    style.ItemSpacing      = {8, 8};
    style.ItemInnerSpacing = {6, 4};
    style.DisabledAlpha    = 0.3f;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.f, 0.f, 0.f, 0.94f);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(
        (LUMI_ASSETS_DIR "/fonts/NotoSansCJKsc-Medium.otf"), 28.0f, NULL,
        io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    //io.FontGlobalScale = 0.5;
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);}
}

void VulkanRHI::ImGuiInit() {
    // 1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, 
    // but it's copied from imgui demo itself.
    std::vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 1000;
    pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
    pool_info.pPoolSizes    = pool_sizes.data();

    VkDescriptorPool imguiPool{};
    VK_CHECK(vkCreateDescriptorPool(device_, &pool_info, nullptr, &imguiPool));

    // 2: initialize imgui library
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiSetStyle();
    
    ImGuiWindowInit();

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance       = instance_;
    init_info.PhysicalDevice = physical_device_;
    init_info.Device         = device_;
    init_info.Queue          = graphics_queue_;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount  = 3;
    init_info.ImageCount     = 3;
    init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info, render_pass_);

    // execute a gpu command to upload imgui font textures
    ImmediateSubmit(
        [](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    // add the destroy the imgui created structures
    destruction_queue_default_.Push([=]() {
        vkDestroyDescriptorPool(device_, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
        ImGuiWindowShutdown();
        ImGui::DestroyContext();
    });
}

void VulkanRHI::GUIPass(VkCommandBuffer cmd_buffer) {
    ImGui_ImplVulkan_NewFrame();
    ImGuiWindowNewFrame();
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
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buffer);
}

}  // namespace lumi