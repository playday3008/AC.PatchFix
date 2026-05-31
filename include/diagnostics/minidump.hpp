#pragma once

#include <Windows.h>

namespace diagnostics {
    void write_minidump(EXCEPTION_POINTERS *ep);
} // namespace diagnostics
