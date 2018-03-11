#include "stdafx.h"
#include "main_window.h"

#include "Resource.h"
#include "abount_dialog.h"
#include <assert.h>
#include <string>

#include "messages.h"
#include "configuration_manager.h"
#include "last_error.h"
#include "canvas_window.h"
#include "application.h"

namespace xerxes
{
    ATOM main_window::_registration = 0;
    HWND main_window::_wnd = NULL;

    LRESULT CALLBACK main_window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);
                // Parse the menu selections:
                switch (wmId)
                {
                case IDM_ABOUT:
                    xerxes::about_dialog::try_show(hWnd);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                case ID_WINDOW_DISPLAYSHOWWINDOW:
                    show_canvas_window();
                    break;
                case ID_WINDOW_CLOSESHOWWINDOW:
                    close_canvas_window();
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                // TODO: Add any drawing code that uses hdc here...
                EndPaint(hWnd, &ps);
            }
            break;
        case WM_USER_CANVAS_WINDOW_CLOSED:
            if (MessageBoxW(_wnd, L"Do you want to show it again?", L"Show window closed", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
                show_canvas_window();
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_DISPLAYCHANGE:
            MessageBoxA(NULL, "A change in the monitor settings has been detected. Please restart the aplication", "Monitor settings changed", MB_OK);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    auto main_window::try_register_class() -> bool
    {
        assert(_registration == 0);

        WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = application::instance();
        wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_XERXESVIEW));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_XERXESVIEW);
        wcex.lpszClassName = MAIN_WINDOW_CLASS_NAME;
        wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

        _registration = RegisterClassExW(&wcex);

        return _registration != 0;
    }

    auto main_window::try_create_window(int x, int y, int width, int height) -> bool
    {
        WCHAR initial_title[MAX_INITIAL_TITLE_LENGTH + 1];
        if (LoadStringW(application::instance(), IDS_APP_TITLE, initial_title, MAX_INITIAL_TITLE_LENGTH) == 0) return false;
        std::wstring title(initial_title);
        title.append(L" - Control");
        _wnd = CreateWindowW(MAIN_WINDOW_CLASS_NAME, title.c_str(), WS_OVERLAPPEDWINDOW,
            x, y, width, height, nullptr, nullptr, application::instance(), nullptr);

        return _wnd != 0;
    }

    auto main_window::show_canvas_window() -> void
    {
        if (!configuration_manager::try_show_canvas_window()) {
            MessageBoxW(_wnd, xerxes::get_last_error::get_error_msg(L"Unknown error").c_str(), L"Failure creating the canvas window", MB_OK);
        }
    }

    auto main_window::close_canvas_window() -> void
    {
        canvas_window::close();
    }

    auto main_window::try_show(int x, int y, int width, int height, int nCmdShow, bool update_immediately) -> bool
    {
        assert(application::instance() != NULL);
        if (!try_register_class()) return false;
        if (!try_create_window(x, y, width, height)) return false;
        ShowWindow(_wnd, nCmdShow);
        if (update_immediately) {
            UpdateWindow(_wnd);
        }

        return true;
    }

}
