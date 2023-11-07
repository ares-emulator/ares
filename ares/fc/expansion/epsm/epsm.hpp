struct EPSM : Expansion, Thread {
  Node::Audio::Stream streamFM;
  Node::Audio::Stream streamSSG;
  
  EPSM(Node::Port);
  ~EPSM();

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto read1() -> n1 override;
  auto read2() -> n5 override;
  auto write(n8 data) -> void override;

  auto writeIO(n16 address, n8 data) -> void override;

private:
  class Interface : public ymfm::ymfm_interface {
  public:
    Interface(EPSM& self) : self{self} {}

    void timerCallback(uint32_t timer) { m_engine->engine_timer_expired(timer); }
    void ymfm_set_busy_end(uint32_t clocks) override { self.busyCyclesRemaining = clocks; }
    bool ymfm_is_busy() override { return self.busyCyclesRemaining > 0; }
    void ymfm_update_irq(bool asserted) override;

    void ymfm_set_timer(uint32_t timer, int32_t duration) override {
      if (duration < 0) {
        self.timerCyclesRemaining[timer] = 0;
      } else {
        self.timerCyclesRemaining[timer] = duration;
      }
    }

    EPSM& self;
  } interface{*this};

  n1 latch;
  n2 ymAddress;
  n8 ymData;
  n16 ioAddress;

  ymfm::ymf288 ymf288;
  s32 busyCyclesRemaining = 0;
  s32 timerCyclesRemaining[2] = {0, 0};
  s32 clocksPerSample = 0;
};
