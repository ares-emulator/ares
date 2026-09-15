auto MDEC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(fifo.input);
  s(fifo.output);

  s(status.remaining.value);
  s(status.currentBlock);
  s(status.outputMaskBit);
  s(status.outputSigned);
  s(status.outputDepth);
  s(status.outputRequest);
  s(status.inputRequest);

  s((u32&)io.mode);
  s((u32&)io.decodePhase);
  s((u32&)io.blockPhase);
  s(io.offset);
  s(io.outputOffset);
  s(io.outputBlock);
  s(io.outputWriteOffset);
  s(io.coefficient);
  s(io.qfactor);
  s(io.phaseClocks);

  s(block.luma);
  s(block.chroma);
  s(block.scale);
  s(block.cr);
  s(block.cb);
  s(block.y0);
  s(block.y1);
  s(block.y2);
  s(block.y3);

  s(output);
}
