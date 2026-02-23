//Yamaha YM2610

struct OPNB : Thread {
  OPNB() : ym2610(interface) {};
  Node::Object node;
  Node::Audio::Stream streamFM;
  Node::Audio::Stream streamSSG;

  //opnb.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  auto read(n2 address) -> n8;
  auto write(n2 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  auto readPCMA(u32 address) -> u8;
  auto readPCMB(u32 address) -> u8;
private:
  class Interface : public ymfm::ymfm_interface {
  public:
    Interface(OPNB& self) : self{self} {}

    void timerCallback(uint32_t timer) { m_engine->engine_timer_expired(timer); }
    void ymfm_set_busy_end(uint32_t clocks) override { self.busyCyclesRemaining = clocks; }
    bool ymfm_is_busy() override { return self.busyCyclesRemaining > 0; }
    void ymfm_update_irq(bool asserted) override { apu.irq.pending = asserted; }

    void ymfm_set_timer(uint32_t timer, int32_t duration) override {
      if (duration < 0) {
        self.timerCyclesRemaining[timer] = 0;
      } else {
        self.timerCyclesRemaining[timer] = duration;
      }
    }

    uint8_t ymfm_external_read(ymfm::access_class type, uint32_t offset) override {
      if(type == ymfm::ACCESS_ADPCM_A) return self.readPCMA(offset);
      if(type == ymfm::ACCESS_ADPCM_B) return self.readPCMB(offset);
      return 0;
    }

    OPNB& self;
  } interface{*this};

  ymfm::ym2610 ym2610;
  s32 busyCyclesRemaining = 0;
  s32 timerCyclesRemaining[2] = {0, 0};
  s32 clocksPerSample = 0;
};

extern OPNB opnb;
