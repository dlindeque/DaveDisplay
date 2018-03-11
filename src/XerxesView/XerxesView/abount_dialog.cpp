#include "stdafx.h"
#include "abount_dialog.h"

#include "Resource.h"
#include "application.h"

namespace xerxes
{

    INT_PTR CALLBACK about_dialog::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        UNREFERENCED_PARAMETER(lParam);
        switch (message)
        {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        }
        return (INT_PTR)FALSE;
    }

    auto about_dialog::try_show(HWND owner) -> bool
    {
        return DialogBox(application::instance(), MAKEINTRESOURCE(IDD_ABOUTBOX), owner, About) > 0;
    }

}
