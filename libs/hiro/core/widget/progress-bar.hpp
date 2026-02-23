#if defined(Hiro_ProgressBar)
struct mProgressBar : mWidget {
  Declare(ProgressBar)

  auto position() const -> u32;
  auto setPosition(u32 position) -> type&;

//private:
  struct State {
    u32 position = 0;
  } state;
};
#endif
