#if defined(Hiro_Console)

namespace hiro {

struct pConsole : pWidget {
  Declare(Console, Widget)

  auto print(const string& text) -> void;
  auto reset() -> void override;
  auto setBackgroundColor(Color color) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setPrompt(const string& prompt) -> void;

  auto _keyPress(u32 scancode, u32 mask) -> bool;
  auto _seekToEnd() -> void;
  auto _seekToMark() -> void;

  GtkWidget* subWidget = nullptr;
  GtkTextBuffer* textBuffer = nullptr;
  string previousPrompt;
  vector<string> history;
  u32 historyOffset = 0;
};

}

#endif
