auto SDD1::serialize(serializer& s) -> void {
  s(r4800);
  s(r4801);
  s(r4804);
  s(r4805);
  s(r4806);
  s(r4807);
  for(auto& channel : dma) {
    s(channel.address);
    s(channel.size);
  }
  s(dmaReady);
  s(decompressor);
}

auto SDD1::Decompressor::serialize(serializer& s) -> void {
  s(im);
  s(gcd);
  s(bg0);
  s(bg1);
  s(bg2);
  s(bg3);
  s(bg4);
  s(bg5);
  s(bg6);
  s(bg7);
  s(pem);
  s(cm);
  s(ol);
}

auto SDD1::Decompressor::IM::serialize(serializer& s) -> void {
  s(offset);
  s(bitCount);
}

auto SDD1::Decompressor::GCD::serialize(serializer& s) -> void {
}

auto SDD1::Decompressor::BG::serialize(serializer& s) -> void {
  s(mpsCount);
  s(lpsIndex);
}

auto SDD1::Decompressor::PEM::serialize(serializer& s) -> void {
  for(auto& info : contextInfo) {
    s(info.status);
    s(info.mps);
  }
}

auto SDD1::Decompressor::CM::serialize(serializer& s) -> void {
  s(bitplanesInfo);
  s(contextBitsInfo);
  s(bitNumber);
  s(currentBitplane);
  s(previousBitplaneBits);
}

auto SDD1::Decompressor::OL::serialize(serializer& s) -> void {
  s(bitplanesInfo);
  s(r0);
  s(r1);
  s(r2);
}
