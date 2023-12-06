auto FDSTimer::clock() -> void {
  if(!enable || !irq) return;

  if(!counter) {
    pending = 1;
    fds.poll();
    if(repeat) counter = period;
    irq &= repeat;
  } else {
    counter--;
  }
}

auto FDSTimer::read(n16 address, n8 data) -> n8 {
  switch(address) {

  case 0x4030:
    data.bit(0) = pending;
    pending = 0;
    fds.poll();
    return data;

  }

  return data;
}

auto FDSTimer::write(n16 address, n8 data) -> void {
  switch(address) {

  case 0x4020:
    period.byte(0) = data;
    return;

  case 0x4021:
    period.byte(1) = data;
    return;

  case 0x4022:
    if(!enable) return;
    repeat = data.bit(0);
    irq = data.bit(1);
    if(irq) {
      counter = period;
    } else {
      pending = 0;
      fds.poll();
    }
    return;

  case 0x4023:
    enable = data.bit(0);
    if(!enable) {
      pending = 0;
      fds.poll();
    }
    return;

  }
}
