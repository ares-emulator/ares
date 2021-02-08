#if defined(Hiro_Action)
struct mAction : mObject {
  Declare(Action)

  auto remove() -> type& override;

//private:
  struct State {
  } state;
};
#endif
