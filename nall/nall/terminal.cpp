#include <nall/terminal.hpp>

namespace nall::terminal {

bool _escapable = false;

NALL_HEADER_INLINE auto redirectStdioToTerminal(bool create) -> void {
#if defined(PLATFORM_WINDOWS)
  if(create) {
    FreeConsole();
    if(!AllocConsole()) return;
  } else if(!AttachConsole(ATTACH_PARENT_PROCESS)) {
    return;
  }

  //unless a new terminal was requested, do not reopen already valid handles (allow redirection to/from file)
  if(create || _get_osfhandle(_fileno(stdin )) < 0) freopen("CONIN$" , "r", stdin );
  if(create || _get_osfhandle(_fileno(stdout)) < 0) freopen("CONOUT$", "w", stdout);
  if(create || _get_osfhandle(_fileno(stderr)) < 0) freopen("CONOUT$", "w", stderr);

  SetConsoleOutputCP(CP_UTF8);

  //enable VT100 escape sequences in the console (Windows 10 build 1511+)
  //silently fails with no ill effects on earlier versions.
  bool escapable = false;

  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
  DWORD dwMode = 0;

  if(hOut != INVALID_HANDLE_VALUE) {
    if(GetConsoleMode(hOut, &dwMode)) {
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      if(SetConsoleMode(hOut, dwMode)) escapable = true;
    }
  }

  if(hErr != INVALID_HANDLE_VALUE) {
    if (GetConsoleMode(hErr, &dwMode)) {
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      if (SetConsoleMode(hErr, dwMode)) escapable = true;
    }
  }

  setEscapable(escapable);
#endif
}

}
