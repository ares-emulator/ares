#define NALL_MAIN_IMPL
#include <nall/main.hpp>

#if defined(PLATFORM_WINDOWS)
  #include <nall/windows/windows.hpp>
  #include <objbase.h>
  #include <shellapi.h>
  #include <winsock2.h>
#endif

namespace nall {

auto main(int argc, char** argv) -> int {
  #if defined(PLATFORM_WINDOWS)
  CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  WSAData wsaData{0};
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  _setmode(_fileno(stdin ), O_BINARY);
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stderr), O_BINARY);
  #endif

  main(Arguments{argc, argv});

  #if defined(PLATFORM_WINDOWS)
  WSACleanup();
  CoUninitialize();
  #endif
  return EXIT_SUCCESS;
}

}

#undef NALL_MAIN_IMPL
