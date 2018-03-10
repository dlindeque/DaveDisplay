#pragma once

#include <windows.h>

#define CANVAS_WINDOW_CLASS_NAME L"DaveDisplayCanvasWindow"

namespace dave
{
    class canvas_window {
    private:
        static ATOM _registration;
        static HWND _wnd;

        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        static auto try_register_class() -> bool;
        static auto try_create_window(int x, int y, int width, int height, bool fullscreen) -> bool;
    public:
        // Try to register the class of this window, create the window and show it. Returns true on success, or false on failure. Use GetLastError and FormatMessage to get the actual error message.
        static auto try_show(int x, int y, int width, int height, int nCmdShow, bool fullscreen = true, bool update_immediately = true) -> bool;
        static auto close() -> void;

        // No instances possible
        canvas_window() = delete;
    };
}