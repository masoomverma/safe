#pragma once

namespace safe {
namespace ui {

class UI {
public:
    // Initialize UI system
    static bool Initialize();
    
    // Render main UI
    static void Render();
    
    // Cleanup UI resources
    static void Cleanup();
    
private:
    static bool s_initialized;
};

} // namespace ui
} // namespace safe
