auto SH2::DMAC::run() -> void {
  for(auto c : range(2)) {
    if(chcr[c].de && !chcr[c].te && dmaor.dme && !dmaor.nmif && !dmaor.ae) {
      if(chcr[c].ar) {
        transfer(c);
      } else if(drcr[c] == 0) {
        if(dreq) transfer(c);
      } else if(drcr[c] == 1) {
        debug(unimplemented, "[SH2] DMA RXI");
      } else if(drcr[c] == 2) {
        debug(unimplemented, "[SH2] DMA TXI");
      }
    }
  }
}

auto SH2::DMAC::transfer(bool c) -> void {
  switch(chcr[c].ts) {

  case 0:  //8-bit
    self->writeByte(dar[c], self->readByte(sar[c]));
    break;
  case 1:  //16-bit
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(dar[c] & 1 || sar[c] & 1)) {
        self->exceptions |= AddressErrorDMA;
        break;
      }
    }
    self->writeWord(dar[c], self->readWord(sar[c]));
    break;
  case 2:  //32-bit
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(dar[c] & 3 || sar[c] & 3)) {
        self->exceptions |= AddressErrorDMA;
        break;
      }
    }
    self->writeLong(dar[c], self->readLong(sar[c]));
    break;
  case 3:  //32-bit x4
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(dar[c] & 3 || sar[c] & 3)) {
        self->exceptions |= AddressErrorDMA;
        break;
      }
    }
    self->writeLong(dar[c] +  0, self->readLong(sar[c] +  0));
    self->writeLong(dar[c] +  4, self->readLong(sar[c] +  4));
    self->writeLong(dar[c] +  8, self->readLong(sar[c] +  8));
    self->writeLong(dar[c] + 12, self->readLong(sar[c] + 12));
    sar[c] += 16;  //always increments regardless of chcr[c].sm
    tcr[c] -= 3;   //always decrements by four instead of by one
    break;
  }

  static constexpr u32 ssize[] = {1, 2, 4, 0};
  if(chcr[c].sm == 1) sar[c] += ssize[chcr[c].ts];
  if(chcr[c].sm == 2) sar[c] -= ssize[chcr[c].ts];

  static constexpr u32 dsize[] = {1, 2, 4, 16};
  if(chcr[c].dm == 1) dar[c] += dsize[chcr[c].ts];
  if(chcr[c].dm == 2) dar[c] -= dsize[chcr[c].ts];

  if(!--tcr[c]) {
    chcr[c].te = 1;
    if(chcr[c].ie) {
      pendingIRQ |= 1 << c;
    }
  }
}
