struct PIDeviceTiming {
  n8 latency;
  n8 pulseWidth;
  n2 releaseDuration;

  auto fasterThan(const PIDeviceTiming& other) const -> bool {
    return latency         >= other.latency
        && pulseWidth      >= other.pulseWidth
        && releaseDuration >= other.releaseDuration;
  }

  //Each PI bus cycle is one RCP clock (~16ns @ 62.5MHz).
  //The PI latches the address on /ALE_L, waits (LAT+1) cycles, then holds /RD (or /WR)
  //low for (PWD+1) cycles and samples the data bus on its rising edge.

  //time the device has from the address being latched until the first word is sampled
  auto accessCycles() const -> u32 {
    return (latency + 1) + (pulseWidth + 1);
  }

  //time the device has to drive the bus while /RD is asserted
  auto pulseCycles() const -> u32 {
    return pulseWidth + 1;
  }
};

struct PIDevice {
  virtual auto piAddress(u32 address, PIDeviceTiming timing) -> bool = 0;
  virtual auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> = 0;
  virtual auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void = 0;
};

struct PIDeviceMemory : PIDevice {
  auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override {
    if(piViewOffset >= piView.size()) return nothing;
    u16 data = *(u16*)&piView[piViewOffset ^ 2];
    piViewOffset += 2;
    return data;
  }

  auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override {
    if(piViewOffset >= piView.size()) return;
    if(piViewWritable) *(u16*)&piView[piViewOffset ^ 2] = data;
    piViewOffset += 2;
  }

  std::span<u8> piView;
  u32 piViewOffset = 0;
  n1  piViewWritable;
  PIDeviceTiming min;
};
