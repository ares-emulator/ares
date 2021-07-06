auto TLCS900H::dma(n2 channel) -> bool {
  auto& source = r.dmas[channel].l.l0;
  auto& target = r.dmad[channel].l.l0;
  auto& length = r.dmam[channel].w.w0;  //0 = 65536
  auto& config = r.dmam[channel].w.w1;

  n32 mode = config.bit(2,4);
  n32 size;
  switch(config.bit(0,1)) {
  case 0: size = Byte; break;
  case 1: size = Word; break;
  case 2: size = Long; break;
  case 3: size = Long; break;  //unknown behavior
  }

  prefetch(6);
  if(mode <= 4) {
    step(2);
    auto data = read(size, source);
    step(4);
    write(size, target, data);
  } else {
    step(4);
  }

  switch(mode) {
  case 0: target += size; break;
  case 1: target -= size; break;
  case 2: source += size; break;
  case 3: source -= size; break;
  case 4:                 break;
  case 5: source += size; break;
  case 6: break;  //unknown behavior
  case 7: break;  //unknown behavior
  }

  return --length == 0;  //true indicates the transfer has completed
}
