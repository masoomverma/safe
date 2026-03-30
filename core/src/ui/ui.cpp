#include "core/include/ui/ui.hpp"

namespace safe::ui {

bool UI::s_initialized = false;

bool UI::Initialize() {
    s_initialized = true;
    return true;
}

void UI::Render() {
    // UI rendering implementation will go here
}

void UI::Cleanup() {
    s_initialized = false;
}

} // namespace space::ui