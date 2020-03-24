struct Thread;

struct Scheduler {
  enum class Mode : uint {
    Run,
    Synchronize,
    SynchronizePrimary,
    SynchronizeAuxiliary,
  };

  Scheduler() = default;
  Scheduler(const Scheduler&) = delete;
  auto operator=(const Scheduler&) = delete;

  inline auto reset() -> void;
  inline auto threads() const -> uint;
  inline auto thread(uint threadID) const -> maybe<Thread&>;
  inline auto uniqueID() const -> uint;
  inline auto minimum() const -> uintmax;
  inline auto maximum() const -> uintmax;

  inline auto append(Thread& thread) -> bool;
  inline auto remove(Thread& thread) -> void;

  inline auto power(Thread& thread) -> void;
  inline auto enter(Mode mode = Mode::Run) -> Event;
  inline auto exit(Event event) -> void;

  inline auto synchronizing() const -> bool;
  inline auto synchronize() -> void;

  inline auto getSynchronize() -> bool;
  inline auto setSynchronize(bool) -> void;

private:
  cothread_t _host = nullptr;     //program thread (used to exit scheduler)
  cothread_t _resume = nullptr;   //resume thread (used to enter scheduler)
  cothread_t _primary = nullptr;  //primary thread (used to synchronize components)
  Mode _mode = Mode::Run;
  Event _event = Event::Step;
  vector<Thread*> _threads;
  bool _synchronize = false;

  friend class Thread;
};

extern Scheduler scheduler;
