// XerxesView.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Resource.h"

#include "main_window.h"
#include "canvas_window.h"
#include "last_error.h"
#include "configuration_manager.h"
#include "application.h"
#include "..\configlib\system_configuration.h"
#include <ShlObj.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    try {
        char papp_data[MAX_PATH];

        if (!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, papp_data))) {
            MessageBoxA(NULL, "Could not find the system APP DATA folder", "Failure initializing the application", MB_OK);
            return -1;
        }

        std::string app_data(papp_data);
        if (app_data.empty()) {
            app_data = ".\\";
        }
        if (!app_data.empty() && app_data[app_data.size() - 1] != '\\') {
            app_data += '\\';
        }

        app_data.append("XerxesView\\");

        CreateDirectoryA(app_data.c_str(), NULL);

        xerxes::system_configuration syscfg(app_data + "syscfg.db");

        xerxes::application::initialize(hInstance, &syscfg);

        try {
            xerxes::configuration_manager::initialize();
        }
        catch (std::exception &ex) {
            MessageBoxA(NULL, ex.what(), "Failure creating the main window", MB_OK);
            return -1;
        }

        if (!xerxes::configuration_manager::try_show_canvas_window()) {
            MessageBoxW(NULL, xerxes::get_last_error::get_error_msg(L"Unknown error").c_str(), L"Failure creating the canvas window", MB_OK);
            return -1;
        }

        if (!xerxes::configuration_manager::try_show_main_window(nCmdShow)) {
            MessageBoxW(NULL, xerxes::get_last_error::get_error_msg(L"Unknown error").c_str(), L"Failure creating the main window", MB_OK);
            return -1;
        }

        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_XERXESVIEW));

        MSG msg;

        // Main message loop:
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        return (int)msg.wParam;
    }
    catch (std::exception &ex) {
        MessageBoxA(NULL, ex.what(), "Failure during program execution", MB_OK);
        return -1;
    }
}
