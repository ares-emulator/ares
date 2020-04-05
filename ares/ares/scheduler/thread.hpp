struct Scheduler;

struct Thread {
  enum : uintmax { Second = (uintmax)-1 >> 1 };
  enum : uintmax { Size = 8_KiB * sizeof(void*) };

  struct EntryPoint {
    cothread_t handle = nullptr;
    function<void ()> entryPoint;
  };

  static auto EntryPoints() -> vector<EntryPoint>&;
  static auto Enter() -> void;

  Thread() = default;
  Thread(const Thread&) = delete;
  auto operator=(const Thread&) = delete;
  virtual ~Thread();

  explicit operator bool() const { return _handle; }
  auto active() const -> bool;
  auto handle() const -> cothread_t;
  auto frequency() const -> uintmax;
  auto scalar() const -> uintmax;
  auto clock() const -> uintmax;

  auto setHandle(cothread_t handle) -> void;
  auto setFrequency(double frequency) -> void;
  auto setScalar(uintmax scalar) -> void;
  auto setClock(uintmax clock) -> void;

  auto create(double frequency, function<void ()> entryPoint) -> void;
  auto destroy() -> void;

  auto step(uint clocks) -> void;
  auto synchronize() -> void;
  template<typename... P> auto synchronize(Thread&, P&&...) -> void;

  auto serialize(serializer& s) -> void;

protected:
  cothread_t _handle = nullptr;
  uint _uniqueID = 0;
  uintmax _frequency = 0;
  uintmax _scalar = 0;
  uintmax _clock = 0;

  friend class Scheduler;
};
