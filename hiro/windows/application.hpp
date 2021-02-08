#if defined(Hiro_Application)

namespace hiro {

struct pApplication {
  static auto exit() -> void;
  static auto modal() -> bool;
  static auto run() -> void;
  static auto pendingEvents() -> bool;
  static auto processEvents() -> void;
  static auto quit() -> void;
  static auto setScreenSaver(bool screenSaver) -> void;

  static auto initialize() -> void;

  struct State {
    vector<pMenuBar*> menuBarsToRebuild;  //deferred menu bars to update
    s32 modalCount = 0;                   //number of modal loops
    Timer modalTimer;                     //to run Application during modal events
    pToolTip* toolTip = nullptr;          //active toolTip
  };
  static auto state() -> State&;
};

}

#endif
