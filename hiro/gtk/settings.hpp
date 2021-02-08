namespace hiro {

struct Settings {
  Settings();
  ~Settings();

  vector<u16> keycodes;

  struct Geometry {
    s32 frameX = 4;
    s32 frameY = 24;
    s32 frameWidth = 8;
    s32 frameHeight = 28;
    s32 menuHeight = 8;
    s32 statusHeight = 4;
  } geometry;

  struct Theme {
    bool actionIcons = true;
    bool widgetColors = true;
  } theme;
};

static Settings settings;

}
