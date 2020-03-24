//analog-to-digital converter

auto CPU::ADC::step(uint clocks) -> void {
  if(!busy) return;
  counter += clocks;
  if(counter >= (!speed ? 160 : 320)) {
    switch(channel) {
    case 0: result[0] = 1023; break;  //battery level (below 528 will not boot)
    case 1: result[1] = 1023; break;  //unknown
    case 2: result[2] = 1023; break;  //unknown
    case 3: result[3] = 1023; break;  //unknown
    }
    if(!repeat) {
      busy = 0;
    } else {
      counter -= !speed ? 160 : 320;
    }
    end = 1;
    cpu.intad.trigger();
  }
}
