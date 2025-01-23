auto NAND::serialize(serializer& s) -> void {
  s(num);
  s(pageOffset);
  s(data);
  s(spare);
  s(writeBuffers);
  s(writeBufferSpares);
  s(eraseQueueOccupied);
  s(eraseQueueAddrs);
  s(writeBuffersOccupied);
  s(writeBufferAddrs);
}
