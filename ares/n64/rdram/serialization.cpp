auto RDRAM::serialize(serializer& s) -> void {
  s(ram);
  s(io.config);
  s(io.deviceID);
  s(io.delay);
  s(io.mode);
  s(io.refreshInterval);
  s(io.refreshRow);
  s(io.rasInterval);
  s(io.minInterval);
  s(io.addressSelect);
  s(io.deviceManufacturer);
}
