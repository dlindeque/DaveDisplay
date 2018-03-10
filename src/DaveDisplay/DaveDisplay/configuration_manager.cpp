#include "stdafx.h"
#include "configuration_manager.h"
#include "main_window.h"
#include "canvas_window.h"
#include "application.h"

#include <exception>
#include <map>
#include <assert.h>

namespace dave
{
    std::vector<configuration_manager::display_info> configuration_manager::_displays;
    configuration_manager::config configuration_manager::_configuration;
    configuration_manager::display_info configuration_manager::_main_display_info;
    configuration_manager::display_info configuration_manager::_canvas_display_info;

    BOOL configuration_manager::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
    {
        configuration_manager::display_info di;
        di.monitor = hMonitor;
        di.rect = *lprcMonitor;

        MONITORINFOEX info;
        info.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfoW(hMonitor, &info);
        if (info.dwFlags && MONITORINFOF_PRIMARY == MONITORINFOF_PRIMARY) {
            di.is_primary = true;
        }
        else {
            di.is_primary = false;
        }

        di.work = info.rcWork;
        di.name = info.szDevice;

        ((std::vector<display_info>*)dwData)->push_back(std::move(di));

        return TRUE;
    }

    auto configuration_manager::read_display_information(std::vector<display_info> &displays) -> void
    {
        if (EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&displays) == 0) throw std::exception("Failure enumerating displays");
    }

    auto configuration_manager::read_configuration_from_database() -> void
    {
        // Try to read configuration - on failure we'll just load defaults
        auto& cfg = _configuration;
        application::get_syscfg()->get_window_configuration([&cfg](const std::wstring &key) -> window_config* { if (key == L"main") return &cfg.main_window; else if (key == L"canvas") return &cfg.canvas_window; else return nullptr; });
    }

    auto configuration_manager::save_configuration_to_database() -> void
    {
        // Try to write configuration
        application::get_syscfg()->write_window_configuration(L"main", _configuration.main_window);
        application::get_syscfg()->write_window_configuration(L"canvas", _configuration.canvas_window);
    }

    auto configuration_manager::initialize() -> void
    {
        // Set default configuration
        _configuration.canvas_window.show_fullscreen = true;
        _configuration.main_window.monitor_name.clear();
        _configuration.canvas_window.monitor_name.clear();

        _displays.clear();
        read_display_information(_displays);
        read_configuration_from_database();

        // Ensure only one window is on primary
        if (_configuration.main_window.show_on_primary && _configuration.canvas_window.show_on_primary) {
            _configuration.canvas_window.show_on_primary = false;
        }

        // Find the primary display
        display_info *primary_display = nullptr;
        {
            for (auto &display : _displays) {
                if (display.is_primary) {
                    primary_display = &display;
                    break;
                }
            }
        }
        if (primary_display == nullptr) {
            // No primary display - this is unusual - pick one
            if (_displays.empty()) {
                // No displays
                throw std::exception("No displays found!");
            }
            else {
                _displays.front().is_primary = true;
                primary_display = &_displays.front();
            }
        }

        // Find the best non-primary monitor
        display_info *best_non_primary = nullptr;
        {
            std::map<size_t, display_info*> ordered;
            for (auto &display : _displays) {
                if (!display.is_primary) {
                    ordered.emplace((display.rect.right - display.rect.left) * (display.rect.bottom - display.rect.top), &display);
                }
            }
            if (!ordered.empty()) {
                best_non_primary = ordered.rbegin()->second;
            }
        }

        // Find each screen's display
        display_info *main_di = nullptr, *canvas_di = nullptr;
        if (_configuration.main_window.show_on_primary) {
            // Find the display for the canvas
            main_di = primary_display;
            if (!_configuration.canvas_window.monitor_name.empty()) {
                for (auto &display : _displays) {
                    if (display.is_primary) continue;
                    if (display.name == _configuration.canvas_window.monitor_name) {
                        canvas_di = &display;
                        break;
                    }
                }
                if (canvas_di == nullptr) {
                    // we couldn't find a display - use the biggest
                    canvas_di = best_non_primary;
                    if (best_non_primary != nullptr) {
                        _configuration.canvas_window.monitor_name = best_non_primary->name;
                    }
                }
            }
            else {
                canvas_di = best_non_primary;
                if (best_non_primary != nullptr) {
                    _configuration.canvas_window.monitor_name = best_non_primary->name;
                }
            }
            if (canvas_di == nullptr) {
                // Only a single display
                canvas_di = primary_display;
            }
        }
        else if (_configuration.canvas_window.show_on_primary) {
            // Find the display for the canvas
            canvas_di = primary_display;
            if (!_configuration.main_window.monitor_name.empty()) {
                for (auto &display : _displays) {
                    if (display.is_primary) continue;
                    if (display.name == _configuration.main_window.monitor_name) {
                        main_di = &display;
                        break;
                    }
                }
                if (main_di == nullptr) {
                    // we couldn't find a display - use the biggest
                    main_di = best_non_primary;
                    if (best_non_primary != nullptr) {
                        _configuration.main_window.monitor_name = best_non_primary->name;
                    }
                }
            }
            else {
                main_di = best_non_primary;
                if (best_non_primary != nullptr) {
                    _configuration.main_window.monitor_name = best_non_primary->name;
                }
            }
            if (main_di == nullptr) {
                // Only a single display
                main_di = primary_display;
            }
        }
        else {
            // Neither on primary
            if (!_configuration.main_window.monitor_name.empty()) {
                for (auto &display : _displays) {
                    if (display.name == _configuration.main_window.monitor_name) {
                        main_di = &display;
                        break;
                    }
                }
            }
            if (!_configuration.canvas_window.monitor_name.empty()) {
                for (auto &display : _displays) {
                    if (display.name == _configuration.canvas_window.monitor_name) {
                        canvas_di = &display;
                        break;
                    }
                }
            }
            if (main_di == nullptr) {
                main_di = primary_display;
                _configuration.main_window.show_on_primary = true;
            }
            if (canvas_di == nullptr) {
                canvas_di = best_non_primary;
                if (canvas_di == nullptr) {
                    canvas_di = primary_display;
                }
                else {
                    _configuration.canvas_window.monitor_name = best_non_primary->name;
                }
            }
        }

        if (_configuration.main_window.show_on_primary) {
            _configuration.main_window.monitor_name.clear();
        }
        if (_configuration.canvas_window.show_on_primary) {
            _configuration.canvas_window.monitor_name.clear();
        }

        assert(main_di != nullptr);
        assert(canvas_di != nullptr);

        save_configuration_to_database();

        _main_display_info = *main_di;
        _canvas_display_info = *canvas_di;
    }

    auto configuration_manager::try_show_main_window(int nCmdShow) -> bool
    {
        if (_main_display_info.is_primary) {
            if (_configuration.main_window.show_maximized) {
                return main_window::try_show(CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, SW_MAXIMIZE);
            }
            else {
                return main_window::try_show(CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nCmdShow);
            }
        }
        else {
            // We can only show maximized on non-primary
            return main_window::try_show(_main_display_info.rect.left, _main_display_info.rect.top, _main_display_info.rect.right-_main_display_info.rect.left, _main_display_info.rect.bottom - _main_display_info.rect.top, SW_MAXIMIZE);
        }
    }

    auto configuration_manager::try_show_canvas_window() -> bool
    {
        if (_configuration.canvas_window.show_fullscreen) {
            return canvas_window::try_show(_canvas_display_info.rect.left, _canvas_display_info.rect.top, _canvas_display_info.rect.right - _canvas_display_info.rect.left, _canvas_display_info.rect.bottom - _canvas_display_info.rect.top, SW_SHOW, _configuration.canvas_window.show_fullscreen);
        }

        if (_canvas_display_info.is_primary) {
            if (_configuration.canvas_window.show_maximized) {
                return canvas_window::try_show(CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, SW_MAXIMIZE, false);
            }
            else {
                return canvas_window::try_show(0, 0, _canvas_display_info.rect.right, _canvas_display_info.rect.bottom, SW_SHOW, false);
            }
        }
        else {
            // We can only show maximized on non-primary
            return canvas_window::try_show(_canvas_display_info.rect.left, _canvas_display_info.rect.top, _canvas_display_info.rect.right - _canvas_display_info.rect.left, _canvas_display_info.rect.bottom - _canvas_display_info.rect.top, SW_MAXIMIZE, false);
        }
    }

}
