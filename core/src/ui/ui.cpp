#include "ui/ui.hpp"
#include "imgui.h"

namespace safe::ui {

    // Forward Declaration of internal functions
    static void RenderMainLayout();

    static void RenderTopBar();
    static void RenderSidebar();
    static void RenderMainPanel();
    static void RenderStatusBar();

    static void RenderFolderList();
    static void RenderFolderDetails();
    static void RenderActionButtons();

    // Public API
    bool UI::s_initialized = false;

    bool UI::Initialize() {
        s_initialized = true;
        return true;
    }

    void UI::Render() {
        // UI rendering implementations
        RenderMainLayout();
    }

    void UI::Cleanup() {
        s_initialized = false;
    }


    // Internal functions Implementations
    static void RenderMainLayout()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGui::Begin("Safe", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoMove
                     );

        const float availableHeight = ImGui::GetContentRegionAvail().y - 8;

        constexpr float topBarHeight = 30.0f;
        constexpr float statusBarHeight = 30.0f;

        const float middleHeight = availableHeight - topBarHeight - statusBarHeight;

        // Top Bar
        ImGui::BeginChild("TopBar", ImVec2(0, topBarHeight), true);
        RenderTopBar();
        ImGui::EndChild();

        // Middle Area
        ImGui::BeginChild("Middle", ImVec2(0, middleHeight), true);

        // Sidebar (Left)
        ImGui::BeginChild("Sidebar", ImVec2(200, 0), true);
        ImGui::Text("Sidebar");
        ImGui::Separator();
        RenderSidebar();
        ImGui::EndChild();

        ImGui::SameLine();

        // Main Panel (Right)
        ImGui::BeginChild("MainPanel", ImVec2(0, 0), true);
        ImGui::Text("Folder Info");
        ImGui::Separator();
        RenderMainPanel();
        ImGui::EndChild();

        ImGui::EndChild();

        // Status Bar
        ImGui::BeginChild("StatusBar", ImVec2(0, statusBarHeight), true);
        ImGui::Text("Status: ");
        RenderStatusBar();
        ImGui::EndChild();

        ImGui::End();
    }

    static void RenderTopBar()
    {
        ImGui::Text("Safe - Secure Folder Manager");
        // Add your action buttons here

    }

    static void RenderSidebar()
    {
        RenderFolderList();
    }

    static void RenderMainPanel()
    {
        RenderFolderDetails();
        RenderActionButtons();
    }

    static void RenderStatusBar()
    {
        // ...
    }

    static void RenderFolderList()
    {
        // ...
    }

    static void RenderFolderDetails()
    {
        // ...
    }

    static void RenderActionButtons()
    {
        // ...
    }
} // namespace safe::ui