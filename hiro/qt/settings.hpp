namespace hiro {

struct Settings {
  Settings();
  ~Settings();

  vector<uint16_t> keycodes;

  struct Geometry {
    s32 frameX = 4;
    s32 frameY = 24;
    s32 frameWidth = 8;
    s32 frameHeight = 28;
    s32 menuHeight = 9;
    s32 statusHeight = 9;
  } geometry;
};

static Settings settings;

}
