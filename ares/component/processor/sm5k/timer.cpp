auto SM5K::tick() -> void {
  switch(RC & 3) {
  case 0: break;
  case 1: if(n7 (++DIV)) return; break;
  case 2: if(n15(++DIV)) return; break;
  case 3: if(P1.bit(1) == 0) return; break;
  }

  if(!++RA) {
    RA = RB;
    IFT = 1;
  }
}
