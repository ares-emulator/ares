auto DMA::Debugger::load(Node::Object parent) -> void {
  tracer.dma = parent->append<Node::Debugger::Tracer::Notification>("Transfer", "DMA");
}

auto DMA::Debugger::transfer(u32 channelID) -> void {
  if(tracer.dma->enabled()) {
    static const char* channelName[7] = {
      "MDEC", "MDEC", " GPU", "  CD", " SPU", " PIO", " OTC"
    };

    auto& channel = PlayStation::dma.channels[channelID];
    string output;
    output.append(channel.synchronization != 2 ? "Block:" : "Chain:", channelName[channelID], " ");
    output.append(!channel.direction ? "  To:" : "From:", hex(channel.address, 6L));
    output.append(channel.decrement ? "-" : "+");
    if(channel.synchronization != 2) output.append(" Length:", hex(channel.length, 4L));
    if(channel.synchronization == 1) output.append(" Blocks:", hex(channel.blocks, 4L));
    tracer.dma->notify(output);
  }
}
