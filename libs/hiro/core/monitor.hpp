#if defined(Hiro_Monitor)
struct Monitor {
  Monitor() = delete;

  static auto count() -> u32;
  static auto dpi(maybe<u32> monitor = nothing) -> Position;
  static auto geometry(maybe<u32> monitor = nothing) -> Geometry;
  static auto primary() -> u32;
  static auto workspace(maybe<u32> monitor = nothing) -> Geometry;
};

//DPI scale X
inline auto sx(f32 x) -> f32 {
  //round DPI scalar to increments of 0.5 (eg 1.0, 1.5, 2.0, ...)
  static auto scale = round(Monitor::dpi().x() / 96.0 * 2.0) / 2.0;
  return x * scale;
}

//DPI scale y
inline auto sy(f32 y) -> f32 {
  static auto scale = round(Monitor::dpi().y() / 96.0 * 2.0) / 2.0;
  return y * scale;
}

inline auto operator"" _sx(unsigned long long x) -> f32 { return sx(x); }
inline auto operator"" _sy(unsigned long long y) -> f32 { return sy(y); }
#endif
