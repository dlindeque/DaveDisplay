#pragma once

#include <windows.h>

#define MAIN_WINDOW_CLASS_NAME L"DaveDisplayMainWindow"
#define MAX_INITIAL_TITLE_LENGTH 500

namespace dave
{
    class main_window {
    private:
        static ATOM _registration;
        static HWND _wnd;

        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        static auto try_register_class() -> bool;
        static auto try_create_window(int x, int y, int width, int height) -> bool;
        static auto show_canvas_window() -> void;
        static auto close_canvas_window() -> void;
    public:
        // Try to register the class of this window, create the window and show it. Returns true on success, or false on failure. Use GetLastError and FormatMessage to get the actual error message.
        static auto try_show(int x, int y, int width, int height, int nCmdShow, bool update_immediately = true) -> bool;

        // No instances possible
        main_window() = delete;

        static inline auto get_wnd() -> HWND { return _wnd; }
    };
}