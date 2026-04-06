#ifndef UI_HPP
#define UI_HPP

namespace safe::ui
{
    /**
     * Safe UI System - Phase 3
     * 
     * Manages the user interface for file/folder management
     * Supports:
     * - Multiple item selection
     * - Lock/Unlock operations
     * - Search functionality
     * - State-driven UI updates
     */
    class UI
    {
    public:
        /**
         * Initialize UI system
         * Must be called before Render()
         * @return true on success, false on failure
         */
        static bool Initialize();

        /**
         * Render main UI frame
         * Should be called every frame in the main loop
         */
        static void Render();

        /**
         * Cleanup UI resources
         * Should be called before application shutdown
         */
        static void Cleanup();

    private:
        static bool s_initialized;
    };

} // namespace safe::ui

#endif // UI_HPP
