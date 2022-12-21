#include <nall/main.hpp>

#if defined(PLATFORM_WINDOWS)
  #include <objbase.h>
  #include <shellapi.h>
  #include <winsock2.h>
#endif

namespace nall {

auto main(int argc, char** argv) -> int {
  #if defined(PLATFORM_WINDOWS)
  CoInitialize(0);
  WSAData wsaData{0};
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  _setmode(_fileno(stdin ), O_BINARY);
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stderr), O_BINARY);
  #endif

  main(Arguments{argc, argv});

  #if !defined(PLATFORM_WINDOWS)
  //when a program is running, input on the terminal queues in stdin
  //when terminating the program, the shell proceeds to try and execute all stdin data
  //this is annoying behavior: this code tries to minimize the impact as much as it can
  //we can flush all of stdin up to the last line feed, preventing spurious commands from executing
  //however, even with setvbuf(_IONBF), we can't stop the last line from echoing to the terminal
  auto flags = fcntl(fileno(stdin), F_GETFL, 0);
  fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);  //don't allow read() to block when empty
  char buffer[4096], data = false;
  while(read(fileno(stdin), buffer, sizeof(buffer)) > 0) data = true;
  fcntl(fileno(stdin), F_SETFL, flags);  //restore original flags for the terminal
  if(data) putchar('\r');  //ensures PS1 is printed at the start of the line
  #endif

  return EXIT_SUCCESS;
}

}

#if defined(PLATFORM_WINDOWS) && defined(SUBSYTEM_WINDOWS)

auto WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) -> int {
  //arguments are retrieved later via GetCommandLineW()
  return nall::main(0, nullptr);
}

#else

auto main(int argc, char** argv) -> int {
  return nall::main(argc, argv);
}

#endif
