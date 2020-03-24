auto Flash::serialize(serializer& s) -> void {
//rom.serialize(s);
  s.boolean(modified);
  s.integer(vendorID);
  s.integer(deviceID);
  s.integer(mode);
  s.integer(index);
}

auto Cartridge::serialize(serializer& s) -> void {
  flash[0].serialize(s);
  flash[1].serialize(s);
}
