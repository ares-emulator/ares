inline auto CPU::load(u32& target) const -> u32 {
  if(delay.load[0].target == &target) {
    return delay.load[0].source;
  }
  return target;
}

inline auto CPU::load(u32& target, u32 source) -> void {
  if(delay.load[0].target == &target) {
    delay.load[0].target = nullptr;
  }
  delay.load[1].target = &target;
  delay.load[1].source =  source;
}

inline auto CPU::store(u32& target, u32 source) -> void {
  if(delay.load[0].target == &target) {
    delay.load[0].target = nullptr;
  }
  target = source;
}

template<u32 N>
inline auto CPU::branch(u32 address, bool take) -> void {
  delay.branch[N].slot = true;
  delay.branch[N].take = take;
  delay.branch[N].address = address;
  if constexpr(N == 0) delay.branch[1] = {};
}

inline auto CPU::processDelayLoad() -> void {
  if(delay.load[0].target) {
   *delay.load[0].target = delay.load[0].source;
  }
  delay.load[0] = delay.load[1];
  delay.load[1] = {};
}

inline auto CPU::processDelayBranch() -> void {
  if(delay.branch[1].take) {
    ipu.pd = delay.branch[1].address;
    instructionHook();  //used to implement fast booting and executable side-loading
  }
  delay.branch[0] = delay.branch[1];
  delay.branch[1] = {};
}
