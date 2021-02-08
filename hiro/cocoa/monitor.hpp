#if defined(Hiro_Monitor)

namespace hiro {

struct pMonitor {
  static auto count() -> u32;
  static auto dpi(u32 monitor) -> Position;
  static auto geometry(u32 monitor) -> Geometry;
  static auto primary() -> u32;
  static auto workspace(u32 monitor) -> Geometry;
};

}

#endif
