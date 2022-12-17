#pragma once

#undef  NOMINMAX
#define NOMINMAX

#undef  UNICODE
#define UNICODE

#undef  WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

//target Windows 7
#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#include <sdkddkver.h>

#include <windows.h>
