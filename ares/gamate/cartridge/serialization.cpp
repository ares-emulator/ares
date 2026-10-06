auto Cartridge::serialize(serializer& s) -> void {
  s(protection.unlocked);
  s(protection.failed);
  s(protection.replying);
  s(protection.position);
  s(protection.byte);
  s(protection.sequence);
  s(bank);
  s(multibank);
  s(cardAvailable);
}
