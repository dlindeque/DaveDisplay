#pragma once

#include <string>
#include "..\dblib\sqlite.h"

namespace xerxes
{
    struct window_config {
        bool show_on_primary;
        bool show_maximized;
        bool show_fullscreen;
        std::wstring monitor_name;
    };

    class system_configuration {
    private:
        sqlite_connection _connection;
        sqlite_statement _window_select;
        sqlite_statement _window_find;
        sqlite_statement _window_insert;
        sqlite_statement _window_update;
    public:
        system_configuration(const std::string &fn);

        template<typename _SelectConfig> inline auto get_window_configuration(const _SelectConfig &select_config) -> void {
            _window_select.reset();
            for (auto &row : _window_select) {
                // [key], [show_on_primary], [show_maximized], [show_fullscreen], [monitor_name]
                auto cfg = select_config(row.get_wstring(0));
                if (cfg != nullptr) {
                    cfg->show_on_primary = row.get_int(1) != 0;
                    cfg->show_maximized = row.get_int(2) != 0;
                    cfg->show_fullscreen = row.get_int(3) != 0;
                    cfg->monitor_name = row.get_is_null(4) ? L"" : row.get_wstring(4);
                }
            }
        }

        auto write_window_configuration(const std::wstring &key, const window_config &cfg) -> void;
    };
}