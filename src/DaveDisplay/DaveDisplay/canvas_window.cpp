#include "stdafx.h"
#include "canvas_window.h"

#include "main_window.h"
#include "Resource.h"
#include <assert.h>
#include <string>

#include "messages.h"
#include "application.h"

namespace dave
{
    ATOM canvas_window::_registration = 0;
    HWND canvas_window::_wnd = NULL;

    LRESULT CALLBACK canvas_window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
        case WM_DESTROY:
            PostMessage(main_window::get_wnd(), WM_USER_CANVAS_WINDOW_CLOSED, 0, 0);
            _wnd = NULL;
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    auto canvas_window::try_register_class() -> bool
    {
        assert(_registration == 0);

        WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = application::instance();
        wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_DAVEDISPLAY));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = CANVAS_WINDOW_CLASS_NAME;
        wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

        _registration = RegisterClassExW(&wcex);

        return _registration != 0;
    }

    auto canvas_window::try_create_window(int x, int y, int width, int height, bool fullscreen) -> bool
    {
        assert(_wnd == NULL);
        WCHAR initial_title[MAX_INITIAL_TITLE_LENGTH + 1];
        if (LoadStringW(application::instance(), IDS_APP_TITLE, initial_title, MAX_INITIAL_TITLE_LENGTH) == 0) return false;
        std::wstring title(initial_title);
        title.append(L" - Show");
        if (fullscreen) {
            _wnd = CreateWindowW(CANVAS_WINDOW_CLASS_NAME, title.c_str(), WS_POPUP,
                x, y, width, height, nullptr, nullptr, application::instance(), nullptr);
        }
        else {
            _wnd = CreateWindowW(CANVAS_WINDOW_CLASS_NAME, title.c_str(), WS_OVERLAPPEDWINDOW,
                x, y, width, height, nullptr, nullptr, application::instance(), nullptr);
        }

        return _wnd != 0;
    }

    auto canvas_window::try_show(int x, int y, int width, int height, int nCmdShow, bool fullscreen, bool update_immediately) -> bool
    {
        assert(application::instance() != NULL);
        if (_registration == 0) {
            if (!try_register_class()) return false;
        }
        if (_wnd == NULL) {
            if (!try_create_window(x, y, width, height, fullscreen)) return false;
        }
        if (fullscreen) {
            ShowWindow(_wnd, SW_SHOW);
        }
        else {
            ShowWindow(_wnd, nCmdShow);
        }

        if (update_immediately) {
            UpdateWindow(_wnd);
        }

        return true;
    }

    auto canvas_window::close() -> void
    {
        DestroyWindow(_wnd);
    }

}
