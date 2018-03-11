#pragma once

#include <vector>
#include "..\configlib\system_configuration.h"

namespace xerxes
{
    class configuration_manager {
    private:
        struct display_info {
            HMONITOR monitor;
            RECT rect;
            RECT work;
            bool is_primary;
            std::wstring name;
        };

        struct config {
            window_config main_window;
            window_config canvas_window;
        };

        static std::vector<display_info> _displays;
        static config _configuration;
        static display_info _main_display_info;
        static display_info _canvas_display_info;

        static BOOL CALLBACK MonitorEnumProc(_In_ HMONITOR hMonitor, _In_ HDC hdcMonitor, _In_ LPRECT lprcMonitor, _In_ LPARAM dwData);
        static auto read_display_information(std::vector<display_info> &displays) -> void;
        static auto read_configuration_from_database() -> void;
        static auto save_configuration_to_database() -> void;
    public:
        configuration_manager() = delete;

        static auto initialize() -> void;

        static auto try_show_main_window(int nCmdShow) -> bool;
        static auto try_show_canvas_window() -> bool;
    };
}