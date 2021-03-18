auto SH2::DMAC::run() -> void {
  for(auto c : range(2)) {
    if(chcr[c].de && dreq) {
      u16 data = self->readWord(sar[c]);
      self->writeWord(dar[c], data);
      dar[c] += 2;
      tcr[c] -= 1;
      if(!tcr[c]) {
        chcr[c].de = 0;
        chcr[c].te = 1;
      }
    }
  }
}
