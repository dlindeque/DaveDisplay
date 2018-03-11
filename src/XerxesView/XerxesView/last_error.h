#pragma once

#include <Windows.h>
#include <string>

namespace xerxes
{
    struct get_last_error {
        static auto get_error_code()->DWORD;
        static auto get_error_msg(const std::wstring &default_msg)->std::wstring;
    };
}