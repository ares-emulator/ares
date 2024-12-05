auto NAND::serialize(serializer& s) -> void {
  s(data);
  s(spare);
  s(writeBuffer);
  s(writeBufferSpare);

  s(pageOffset);
  s(eraseQueueOccupied);
  s(eraseQueuePage);
}
