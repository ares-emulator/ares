#pragma once

#include <nall/platform.hpp>
#include <nall/arguments.hpp>
#include <nall/string.hpp>

namespace nall {
  auto main(Arguments arguments) -> void;

  auto main(int argc, char** argv) -> int;
}

#if !defined(NALL_MAIN_IMPL)
#if defined(PLATFORM_WINDOWS) && defined(SUBSYTEM_WINDOWS)

extern "C" {
auto __stdcall WinMain(struct HINSTANCE__* hInstance, struct HINSTANCE__* hPrevInstance, char* pCmdLine, int nCmdShow) -> int {
  //arguments are retrieved later via GetCommandLineW()
  return nall::main(0, nullptr);
}
}

#else

auto main(int argc, char** argv) -> int {
  return nall::main(argc, argv);
}

#endif
#endif

#if defined(NALL_HEADER_ONLY)
  #include <nall/main.cpp>
#endif
