auto Player::serialize(serializer& s) -> void {
  s(status.enable);
  s(status.rumble);

  s(status.logoDetected);
  s(status.logoCounter);

  s(status.packet);
  s(status.send);
  s(status.recv);

  s(status.timeout);
}
