auto Card::serialize(serializer& s) -> void {
  s(ram);
}

auto CardSlot::serialize(serializer& s) -> void {
  if(device) device->serialize(s);
  s(lock);
  s(select);
  s(bank);
}
