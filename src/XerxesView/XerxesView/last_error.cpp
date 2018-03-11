#include "stdafx.h"
#include "last_error.h"

namespace xerxes
{

    auto get_last_error::get_error_code() -> DWORD
    {
        return GetLastError();
    }

    auto get_last_error::get_error_msg(const std::wstring &default_msg) -> std::wstring
    {
        LPVOID buf = NULL;
        auto dw = get_error_code();
        if (dw == 0) {
            return L"No Error";
        }

        if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&buf,
            0,
            NULL) == 0) {

            if (buf != NULL) {
                LocalFree(buf);
            }

            return default_msg;
        }
        else {

            std::wstring result((LPCTSTR)buf);

            LocalFree(buf);

            return result;
        }
    }

}
