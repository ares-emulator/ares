auto APU::Debugger::load(Node::Object parent) -> void {
  properties.ports = parent->append<Node::Debugger::Properties>("Sound I/O");
  properties.ports->setQuery([&] { return ports(); });
}

auto APU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(properties.ports);
  properties.ports.reset();
}

auto APU::Debugger::ports() -> string {
  string output;

  output.append("Headphone Output: ", self.io.headphonesEnable ? "enabled" : "disabled", "\n");
  output.append("Speaker Output: ", self.io.speakerEnable ? "enabled, " : "disabled, ");
  if(self.io.speakerShift == 0) output.append("100%");
  if(self.io.speakerShift == 1) output.append("50%");
  if(self.io.speakerShift == 2) output.append("25%");
  if(self.io.speakerShift == 3) output.append("12.5%");
  output.append("\n");
  output.append("Wavetable: ", hex(self.io.waveBase << 6, 4L), "\n");

  output.append("Channel 1: ");
  if(!self.channel1.io.enable) {
    output.append("disabled");
  } else {
    output.append("tone, ", self.channel1.io.pitch, " (", (96000.0 / (2048 - self.channel1.io.pitch)), " Hz), ",
      self.channel1.io.volumeLeft, "/", self.channel1.io.volumeRight);
  }
  output.append("\n");

  output.append("Channel 2: ");
  if(!self.channel2.io.enable) {
    output.append("disabled");
  } else if(self.channel2.io.voice) {
    output.append("voice, ", (self.channel2.io.volumeRight | ((self.channel2.io.volumeLeft) << 4)), ", ",
      self.channel2.io.voiceEnableLeftFull  ? "100%" : (self.channel2.io.voiceEnableLeftHalf  ? "50%" : "0"), "/",
      self.channel2.io.voiceEnableRightFull ? "100%" : (self.channel2.io.voiceEnableRightHalf ? "50%" : "0"));
  } else {
    output.append("tone, ", self.channel2.io.pitch, " (", (96000.0 / (2048 - self.channel2.io.pitch)), " Hz), ",
      self.channel2.io.volumeLeft, "/", self.channel2.io.volumeRight);
  }
  output.append("\n");

  output.append("Channel 3: ");
  if(!self.channel3.io.enable) {
    output.append("disabled");
  } else {
    output.append("tone, ", self.channel3.io.pitch, " (", (96000.0 / (2048 - self.channel3.io.pitch)), " Hz), ",
      self.channel3.io.volumeLeft, "/", self.channel3.io.volumeRight);
    if(self.channel3.io.sweep) {
      output.append(", sweep by ", self.channel3.io.sweepValue, " every ", self.channel3.io.sweepTime);
    }
  }
  output.append("\n");

  output.append("Channel 4: ");
  if(!self.channel4.io.enable) {
    output.append("disabled");
  } else {
    if(self.channel4.io.noise) {
      output.append("noise, mode ", (self.channel4.io.noiseMode));
    } else {
      output.append("tone, ", self.channel4.io.pitch, " (", (96000.0 / (2048 - self.channel4.io.pitch)), " Hz)");
    }
    output.append(", ", self.channel4.io.volumeLeft, "/", self.channel4.io.volumeRight);
  }
  if(self.channel4.io.noiseUpdate) output.append(", LFSR active");
  output.append("\n");

  if(system.color()) {
    output.append("Hyper Voice: ");
    if(!self.channel5.io.enable) {
      output.append("disabled");
    } else {
      // TODO: Hyper Voice
      output.append("enabled");
    }
    output.append("\n");

    output.append("Sound DMA: ",
      self.dma.io.enable ? "enabled" : "disabled", ", ",
      self.dma.io.loop ? "repeat" : "one-shot", ", ",
      self.dma.io.target ? "Hyper Voice" : "Channel 2", ", ",
      self.dma.io.direction ? "reverse" : "forward",
      "\n");
    output.append("Sound DMA Rate: ");
    if(self.dma.io.rate == 0) output.append("4000 Hz\n");
    if(self.dma.io.rate == 1) output.append("6000 Hz\n");
    if(self.dma.io.rate == 2) output.append("12000 Hz\n");
    if(self.dma.io.rate == 3) output.append("24000 Hz\n");
    output.append("Sound DMA Source: ", hex(self.dma.io.source), " (", self.dma.io.length, " bytes)", "\n");
  }

  return output;
}