#pragma once

#include <Windows.h>
#include "..\configlib\system_configuration.h"

namespace dave
{
    class application {
    private:
        static HINSTANCE _hInstance;
        static system_configuration *_syscfg;
    public:
        static auto initialize(HINSTANCE hInstance, system_configuration *syscfg) -> void { _hInstance = hInstance; _syscfg = syscfg; }
        static auto instance() -> HINSTANCE { return _hInstance; }
        static auto get_syscfg() -> system_configuration* { return _syscfg; }
    };
}