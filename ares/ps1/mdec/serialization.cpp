auto MDEC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(fifo.input);
  s(fifo.output);

  s(status.remaining);
  s(status.currentBlock);
  s(status.outputMaskBit);
  s(status.outputSigned);
  s(status.outputDepth);
  s(status.outputRequest);
  s(status.inputRequest);
  s(status.commandBusy);
  s(status.inputFull);
  s(status.outputEmpty);

  s((u32&)io.mode);
  s(io.offset);

  s(block.luma);
  s(block.chroma);
  s(block.scale);
  s(block.cr);
  s(block.cb);
  s(block.y0);
  s(block.y1);
  s(block.y2);
  s(block.y3);
}
