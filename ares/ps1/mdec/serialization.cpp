auto MDEC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(fifo.input);
  s(fifo.inputBuffer);
  s(fifo.output);
  s(fifo.pendingOutput);

  s(status.remaining);
  s(status.noOperation);
  s(status.noOperationValue);
  s(status.currentBlock);
  s(status.outputMaskBit);
  s(status.outputSigned);
  s(status.outputDepth);
  s(status.outputRequest);
  s(status.inputRequest);

  s((u32&)io.mode);
  s(io.resetPending);
  s(io.offset);
  s(io.outputWord);
  s(io.remaining);
  s(io.decodeBlock);
  s(prefetch.coefficient);
  s(prefetch.block);
  s(prefetch.index);
  s(prefetch.qfactor);
  s(prefetch.started);
  s(prefetch.ready);

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
