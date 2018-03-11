#pragma once

#include <Windows.h>

namespace xerxes
{
    class about_dialog {
    private:
        static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    public:
        static auto try_show(HWND owner) -> bool;
    };
}