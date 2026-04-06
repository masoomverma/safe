/**
 * Safe UI Implementation - Phase 3
 * 
 * SYSTEM SPECIFICATION:
 * - Manages files and folders locally
 * - Supports multiple item selection (Select mode / Ctrl+Click / Shift+Click)
 * - Lock/Unlock operations with encryption (placeholder for now)
 * - Search functionality with case-insensitive matching
 * - State-driven UI with proper button enable/disable logic
 * 
 * DATA MODEL:
 * - Item: {name, isFolder, isLocked}
 * 
 * STATE VARIABLES:
 * - items: Vector of all items in the system
 * - selectedIndices: Set of currently selected item indices
 * - searchBuffer: Current search query
 * - statusMessage: Current status message displayed to user
 * - showLockPopup: Whether lock options popup is visible
 * - combinedNameBuffer: Buffer for combined file name input
 * - selectedLockOption: Current lock option selection (0=Individual, 1=Combined)
 */

#include "ui/ui.hpp"
#include "imgui.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <cctype>

namespace safe::ui
{
    // CONSTANTS
    // UI Layout Constants
    namespace {
        constexpr float TOPBAR_HEIGHT = 41.0f;
        constexpr float STATUSBAR_HEIGHT = 30.0f;
        constexpr float SIDEBAR_WIDTH = 200.0f;
        constexpr float BUTTON_WIDTH = 80.0f;
        constexpr float SELECT_BUTTON_WIDTH = 90.0f;
        constexpr float BUTTON_SPACING = 10.0f;
        constexpr float SEARCH_WIDTH = 200.0f;
        constexpr size_t SEARCH_BUFFER_SIZE = 128;
    }


    // DATA MODEL
    /**
     * Item represents a file or folder in the system
     * - name: Display name of the item
     * - isFolder: true if folder, false if file
     * - isLocked: true if encrypted/locked, false otherwise
     */
    struct Item
    {
        std::string name;
        bool isFolder{};
        bool isLocked{};
    };


    // STATE VARIABLES
    // Items list - currently static, will be replaced with database
    // TODO: Replace with database-loaded items
    static std::vector<Item> items =
    {
        {"Project", true, false},
        {"SecretDocs", true, false},
        {"Images", true, false}
    };

    // Selection tracking - uses unordered_set for O(1) lookup
    static std::unordered_set<size_t> selectedIndices;
    static bool multiSelectMode = false;
    static size_t selectionAnchorIndex = 0;
    static bool hasSelectionAnchor = false;
    
    // Search state
    static char searchBuffer[SEARCH_BUFFER_SIZE] = "";
    
    // Status message displayed in status bar
    static std::string statusMessage = "Ready";
    
    // Lock popup state
    static bool showLockPopup = false;
    static char combinedNameBuffer[256] = "";
    static int selectedLockOption = 0; // 0 = Individual, 1 = Combined


    // FORWARD DECLARATIONS
    // Main rendering functions
    static void RenderMainLayout();
    static void RenderTopBar();
    static void RenderMainPanel();
    static void RenderStatusBar();
    static void RenderFolderList();
    static void RenderFolderDetails();
    static void RenderLockPopup();

    // Helper functions
    static std::string ToLower(const std::string& str);
    static bool IsValidIndex(size_t index);
    static void GetSelectionState(bool& anyLocked, bool& anyUnlocked, bool& hasInvalidSelection);
    
    // Operation handlers
    static void PerformLockOperation();
    static void PerformUnlockOperation();

    // PUBLIC API IMPLEMENTATION
    bool UI::s_initialized = false;

    bool UI::Initialize() {
        s_initialized = true;
        statusMessage = "Ready";
        return true;
    }

    void UI::Render() {
        if (!s_initialized) {
            return;
        }
        RenderMainLayout();
    }

    void UI::Cleanup() {
        s_initialized = false;
        selectedIndices.clear();
        multiSelectMode = false;
        hasSelectionAnchor = false;
        items.clear();
    }


    // HELPER FUNCTIONS
    /**
     * Converts a string to lowercase for case-insensitive comparison
     * Uses C++20 ranges for modern, clean syntax
     */
    static std::string ToLower(const std::string& str) {
        std::string result = str;
        std::ranges::transform(result, result.begin(),
                               [](const unsigned char c) { return std::tolower(c); });
        return result;
    }

    /**
     * Validates that an index is within bounds of the items vector
     * Prevents out-of-bounds access crashes
     */
    static bool IsValidIndex(size_t index) {
        return index < items.size();
    }

    static void GetSelectionState(bool& anyLocked, bool& anyUnlocked, bool& hasInvalidSelection)
    {
        anyLocked = false;
        anyUnlocked = false;
        hasInvalidSelection = false;

        for (const size_t idx : selectedIndices)
        {
            if (!IsValidIndex(idx))
            {
                hasInvalidSelection = true;
                continue;
            }

            if (items[idx].isLocked) anyLocked = true;
            else anyUnlocked = true;
        }
    }


    // UI RENDERING FUNCTIONS
    /**
     * Main layout container
     * Creates fullscreen window with three sections:
     * - TopBar: Search and quick actions
     * - Middle: Sidebar + Main Panel
     * - StatusBar: Status messages
     */
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

        const float availableHeight = ImGui::GetContentRegionAvail().y - 8.0f;
        const float middleHeight = availableHeight - TOPBAR_HEIGHT - STATUSBAR_HEIGHT;

        // Top Bar
        ImGui::BeginChild("TopBar", ImVec2(0, TOPBAR_HEIGHT), true, 
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderTopBar();
        ImGui::EndChild();

        // Middle Area
        ImGui::BeginChild("Middle", ImVec2(0, middleHeight), true);

        // Sidebar (Left)
        ImGui::BeginChild("Sidebar", ImVec2(SIDEBAR_WIDTH, 0), true);
        ImGui::Text("Folders");
        ImGui::Separator();
        RenderFolderList();
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
        ImGui::BeginChild("StatusBar", ImVec2(0, STATUSBAR_HEIGHT), true);
        RenderStatusBar();
        ImGui::EndChild();

        ImGui::End();
        
        // Render popups (must be outside main window)
        RenderLockPopup();
    }

    /**
     * Top Bar Section
     * Contains:
     * - Search input for filtering items
     * - Open button (placeholder for file picker)
     * - Lock button (enabled when all selected are unlocked)
     * - Unlock button (enabled when all selected are locked)
     * 
     * Button State Logic:
     * - No selection: Both disabled
     * - All unlocked: Lock enabled, Unlock disabled
     * - All locked: Lock disabled, Unlock enabled
     * - Mixed: Both disabled
     */

    static void RenderTopBar()
    {
        const float height = ImGui::GetContentRegionAvail().y;
        const float itemHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (height - itemHeight) * 0.01f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

        ImGui::SetNextItemWidth(SEARCH_WIDTH);
        ImGui::InputTextWithHint("##search", "Search...", searchBuffer, SEARCH_BUFFER_SIZE);

        ImGui::SameLine(0, BUTTON_SPACING);

        if (ImGui::Button("Open", ImVec2(BUTTON_WIDTH, 0)))
        {
            // TODO: Implement file browser dialog
            statusMessage = "Open: Not yet implemented";
        }

        ImGui::SameLine(0, BUTTON_SPACING);

        const bool stylePushed = multiSelectMode;
        if (stylePushed)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.70f, 0.95f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.62f, 0.90f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.52f, 0.82f, 1.0f));
        }

        if (ImGui::Button(multiSelectMode ? "Select ON" : "Select", ImVec2(SELECT_BUTTON_WIDTH, 0)))
        {
            multiSelectMode = !multiSelectMode;
            statusMessage = multiSelectMode
                ? "Select mode enabled: click to multi-select (Ctrl+Click also works)"
                : "Select mode disabled (Ctrl+Click still works)";
        }

        if (stylePushed)
        {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine(0, BUTTON_SPACING);

        bool anyLocked = false;
        bool anyUnlocked = false;
        bool hasInvalidSelection = false;
        GetSelectionState(anyLocked, anyUnlocked, hasInvalidSelection);

        if (hasInvalidSelection)
        {
            statusMessage = "Invalid selection detected";
        }

        const bool hasSelection = !selectedIndices.empty();
        const bool canLock = hasSelection && anyUnlocked && !anyLocked;
        const bool canUnlock = hasSelection && anyLocked && !anyUnlocked;

        if (!canLock) ImGui::BeginDisabled();

        if (ImGui::Button("Lock", ImVec2(BUTTON_WIDTH, 0)))
        {
            PerformLockOperation();
        }

        if (!canLock) ImGui::EndDisabled();

        ImGui::SameLine(0, BUTTON_SPACING);

        if (!canUnlock) ImGui::BeginDisabled();

        if (ImGui::Button("Unlock", ImVec2(BUTTON_WIDTH, 0)))
        {
            PerformUnlockOperation();
        }

        if (!canUnlock) ImGui::EndDisabled();

        if (hasSelection && anyLocked && anyUnlocked)
        {
            statusMessage = "Mixed selection: Select either locked or unlocked items";
        }

        ImGui::PopStyleVar();
    }

    /**
     * Main Panel Section
     * Displays folder/file details
     */
    static void RenderMainPanel()
    {
        RenderFolderDetails();
    }

    /**
     * Status Bar Section
     * Displays current status message to user
     */
    static void RenderStatusBar()
    {
        ImGui::Text("Status: %s", statusMessage.c_str());
    }

    /**
     * Folder List (Sidebar)
     * 
     * Displays all items in a single unified list
     * Features:
     * - Search filtering (case-insensitive)
     * - Icon display based on type/state
     * - Multiple selection support
     * - Click to select/deselect
     * - Ctrl+Click: toggle random/non-contiguous selection
     * - Shift+Click: select contiguous range from anchor
     * 
     * Icons:
     * - 🔒 Locked items
     * - 📁 Unlocked folders
     * - 📄 Unlocked files
     */

    static void RenderFolderList()
    {
        // Cache search filter state to avoid repeated strlen calls
        const bool hasSearchFilter = searchBuffer[0] != '\0';
        const std::string lowerSearchText = hasSearchFilter ? ToLower(searchBuffer) : "";
        const bool shiftDown = ImGui::GetIO().KeyShift;
        const bool ctrlDown = ImGui::GetIO().KeyCtrl;

        for (size_t i = 0; i < items.size(); i++)
        {
            // Search filter - case_insensitive
            if (hasSearchFilter)
            {
                if (const std::string lowerName = ToLower(items[i].name); lowerName.find(lowerSearchText) ==
                    std::string::npos)
                    continue;
            }

            const auto& [name, isFolder, isLocked] = items[i];
            const bool selected = selectedIndices.contains(i);

            // Build label with icon
            const char* icon = isLocked ? "🔒 " : (isFolder ? "📁 " : "📄 ");
            const std::string label = std::string(icon) + name;

            ImGui::PushID(static_cast<int>(i));
            const bool clicked = ImGui::Selectable(label.c_str(), selected);
            ImGui::PopID();

            if (clicked)
            {
                if (shiftDown)
                {
                    if (!hasSelectionAnchor || !IsValidIndex(selectionAnchorIndex))
                    {
                        selectionAnchorIndex = i;
                        hasSelectionAnchor = true;
                        selectedIndices.clear();
                        selectedIndices.insert(i);
                    }
                    else
                    {
                        const size_t rangeStart = std::min(selectionAnchorIndex, i);
                        const size_t rangeEnd = std::max(selectionAnchorIndex, i);
                        selectedIndices.clear();
                        for (size_t idx = rangeStart; idx <= rangeEnd; ++idx)
                        {
                            selectedIndices.insert(idx);
                        }
                    }
                }
                else if (ctrlDown || multiSelectMode)
                {
                    if (selected)
                    {
                        selectedIndices.erase(i);
                    }
                    else
                    {
                        selectedIndices.insert(i);
                    }
                    selectionAnchorIndex = i;
                    hasSelectionAnchor = true;
                }
                else
                {
                    selectedIndices.clear();
                    selectedIndices.insert(i);
                    selectionAnchorIndex = i;
                    hasSelectionAnchor = true;
                }
            }
        }
    }

    static void RenderFolderDetails()
    {
        if (selectedIndices.empty())
        {
            ImGui::Text("No item selected");
            ImGui::TextDisabled("Select a file or folder to view details");
            return;
        }

        if (selectedIndices.size() == 1)
        {
            const size_t index = *selectedIndices.begin();
            
            // Bounds check
            if (!IsValidIndex(index))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Invalid selection");
                selectedIndices.clear();
                return;
            }

            const auto& [name, isFolder, isLocked] = items[index];

            ImGui::Text("Name: %s", name.c_str());
            ImGui::Text("Type: %s", isFolder ? "Folder" : "File");
            ImGui::Text("Status: %s", isLocked ? "Locked" : "Unlocked");
            ImGui::Text("Size: --");
            ImGui::Text("Path: --");
            ImGui::Text("Last Modified: --");
            ImGui::Spacing();
            ImGui::Separator();
            return;
        }

        // Multiple selection
        size_t folderCount = 0;
        size_t fileCount = 0;

        for (const size_t idx : selectedIndices)
        {
            if (!IsValidIndex(idx)) continue;
            
            if (items[idx].isFolder) folderCount++;
            else fileCount++;
        }

        ImGui::Text("Selected Items: %zu", selectedIndices.size());
        ImGui::Text("Folders: %zu", folderCount);
        ImGui::Text("Files: %zu", fileCount);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Actions will apply to all selected items");
    }

    // LOCK OPERATION POPUP
    /**
     * Lock Options Popup Modal
     * 
     * Displayed when locking multiple items
     * Two options:
     * 1. Encrypt individually - Each item locked separately
     * 2. Combine into single .safe - Creates combined encrypted file
     * 
     * Validation:
     * - Combined option requires a name to be entered
     * - Empty name shows error message
     */

    static void RenderLockPopup()
    {
        if (!showLockPopup)
            return;

        ImGui::SetNextWindowSize(ImVec2(450, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Lock Options", &showLockPopup, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("You have selected multiple items. Choose how to lock them:");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Option 1: Encrypt individually
            if (ImGui::RadioButton("Encrypt individually", selectedLockOption == 0))
            {
                selectedLockOption = 0;
            }
            ImGui::TextDisabled("Each item will be encrypted separately");
            
            ImGui::Spacing();

            // Option 2: Combine into single .safe
            if (ImGui::RadioButton("Combine into single .safe file", selectedLockOption == 1))
            {
                selectedLockOption = 1;
            }
            ImGui::TextDisabled("All selected items will be combined into one encrypted file");

            if (selectedLockOption == 1)
            {
                ImGui::Spacing();
                ImGui::Text("Combined file name:");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##combinedName", "Enter name (without .safe)", combinedNameBuffer, sizeof(combinedNameBuffer));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons
            constexpr float buttonWidth = 100.0f;
            const float spacing = ImGui::GetStyle().ItemSpacing.x;
            const float totalWidth = (buttonWidth * 2) + spacing;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

            if (ImGui::Button("OK", ImVec2(buttonWidth, 0)))
            {
                bool canProceed = true;

                if (selectedLockOption == 1)
                {
                    // Validate combined name
                    if (strlen(combinedNameBuffer) == 0)
                    {
                        statusMessage = "Error: Please enter a name for the combined file";
                        canProceed = false;
                    }
                }

                if (canProceed)
                {
                    if (selectedLockOption == 0)
                    {
                        // Option 1: Lock individually
                        for (const size_t idx : selectedIndices)
                        {
                            if (!IsValidIndex(idx)) continue;
                            items[idx].isLocked = true;
                        }
                        statusMessage = "Locked selected items individually";
                    }
                    else
                    {
                        // Option 2: Combine into single .safe
                        std::string combinedName = std::string(combinedNameBuffer) + ".safe";

                        // Create new combined item
                        Item combinedItem;
                        combinedItem.name = combinedName;
                        combinedItem.isFolder = false;
                        combinedItem.isLocked = true;

                        // Remove selected items (collect indices first to avoid iterator invalidation)
                        std::vector<size_t> indicesToRemove(selectedIndices.begin(), selectedIndices.end());
                        std::sort(indicesToRemove.rbegin(), indicesToRemove.rend()); // Sort descending

                        for (const size_t idx : indicesToRemove)
                        {
                            if (IsValidIndex(idx))
                            {
                                items.erase(items.begin() + static_cast<std::ptrdiff_t>(idx));
                            }
                        }

                        // Add combined item
                        items.push_back(combinedItem);

                        // Clear selection
                        selectedIndices.clear();

                        statusMessage = "Created combined safe file: " + combinedName;
                    }

                    showLockPopup = false;
                    selectedLockOption = 0;
                    combinedNameBuffer[0] = '\0';
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
            {
                showLockPopup = false;
                selectedLockOption = 0;
                combinedNameBuffer[0] = '\0';
                statusMessage = "Lock operation cancelled";
            }

            ImGui::EndPopup();
        }
    }

    static void PerformLockOperation()
    {
        if (selectedIndices.empty())
        {
            statusMessage = "No items selected";
            return;
        }

        // Check if any item is already locked
        bool anyLocked = false;
        for (const size_t idx : selectedIndices)
        {
            if (!IsValidIndex(idx)) continue;
            if (items[idx].isLocked)
            {
                anyLocked = true;
                break;
            }
        }

        if (anyLocked)
        {
            statusMessage = "Cannot lock: Some items are already locked";
            return;
        }

        // Single item: lock directly
        if (selectedIndices.size() == 1)
        {
            const size_t idx = *selectedIndices.begin();
            if (IsValidIndex(idx))
            {
                items[idx].isLocked = true;
                statusMessage = "Locked selected item";
            }
        }
        else
        {
            // Multiple items: show popup
            showLockPopup = true;
            ImGui::OpenPopup("Lock Options");
        }
    }

    static void PerformUnlockOperation()
    {
        if (selectedIndices.empty())
        {
            statusMessage = "No items selected";
            return;
        }

        // Check if ALL selected items are locked
        bool allLocked = true;
        for (const size_t idx : selectedIndices)
        {
            if (!IsValidIndex(idx)) continue;
            if (!items[idx].isLocked)
            {
                allLocked = false;
                break;
            }
        }

        if (!allLocked)
        {
            statusMessage = "Cannot unlock: Not all selected items are locked";
            return;
        }

        // Unlock all selected items
        for (const size_t idx : selectedIndices)
        {
            if (!IsValidIndex(idx)) continue;
            items[idx].isLocked = false;
        }

        statusMessage = selectedIndices.size() == 1 ? "Unlocked selected item" : "Unlocked selected items";
    }
}
