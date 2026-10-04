auto RDRAM::serialize(serializer& s) -> void {
  s(ram);
  s(mapIdentity);
  for(auto& chip : chips) {
    s(chip.present);
    s(chip.enable);
    s(chip.autoCurrent);
    s(chip.deviceID);
    s(chip.writeDelay);
    s(chip.cci);
    s(chip.ccInternal);
    s(chip.ccLow);
    s(chip.ccHigh);
    s(chip.deviceType);
    s(chip.deviceIDReg);
    s(chip.delay);
    s(chip.mode);
    s(chip.refreshInterval);
    s(chip.refreshRow);
    s(chip.rasInterval);
    s(chip.minInterval);
    s(chip.addressSelect);
    s(chip.deviceManufacturer);
    s(chip.currentControl);
    s(chip.row);
  }
}
