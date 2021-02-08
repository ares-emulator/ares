#if defined(Hiro_Console)

@interface CocoaConsole : NSScrollView {
@public
  phoenix::Console* console;
}
-(id) initWith:(phoenix::Console&)console;
@end

namespace hiro {

struct pConsole : public pWidget {
  Console& console;
  CocoaConsole* cocoaConsole = nullptr;

  auto print(string text) -> void;
  auto reset() -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setPrompt(string prompt) -> void;

  pConsole(Console& console) : pWidget(console), console(console) {}
  auto constructor() -> void;
  auto destructor() -> void;
};

}

#endif
