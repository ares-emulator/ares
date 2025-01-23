auto Aleck64::readPort1() -> u32 {
  n32 value = 0xffff'ffff;

  if(gameConfig->type() == BoardType::E92) {
    value.bit(0)  = !aleck64.controls.p1up->value();
    value.bit(1)  = !aleck64.controls.p1down->value();
    value.bit(2)  = !aleck64.controls.p1left->value();
    value.bit(3)  = !aleck64.controls.p1right->value();
    value.bit(4)  = !aleck64.controls.p1[0]->value();
    value.bit(5)  = !aleck64.controls.p1[1]->value();
    value.bit(6)  = !aleck64.controls.p1[2]->value();
    value.bit(7)  = !aleck64.controls.p1[3]->value();
    value.bit(8)  = !aleck64.controls.p2up->value();
    value.bit(9)  = !aleck64.controls.p2down->value();
    value.bit(10) = !aleck64.controls.p2left->value();
    value.bit(11) = !aleck64.controls.p2right->value();
    value.bit(12) = !aleck64.controls.p2[0]->value();
    value.bit(13) = !aleck64.controls.p2[1]->value();
    value.bit(14) = !aleck64.controls.p2[2]->value();
    value.bit(15) = !aleck64.controls.p2[3]->value();
  } else if(gameConfig->type() == BoardType::E90) {
    value.bit( 0) = !aleck64.controls.p1up->value();
    value.bit( 1) = !aleck64.controls.p1down->value();
    value.bit( 2) = !aleck64.controls.p1left->value();
    value.bit( 3) = !aleck64.controls.p1right->value();
    value.bit( 4) = !aleck64.controls.p1[0]->value();
    value.bit( 5) = !aleck64.controls.p1[1]->value();
    value.bit( 7) = !aleck64.controls.p1start->value();
    value.bit( 8) = !aleck64.controls.p2up->value();
    value.bit( 9) = !aleck64.controls.p2down->value();
    value.bit(10) = !aleck64.controls.p2left->value();
    value.bit(11) = !aleck64.controls.p2right->value();
    value.bit(12) = !aleck64.controls.p2[0]->value();
    value.bit(13) = !aleck64.controls.p2[1]->value();
    value.bit(15) = !aleck64.controls.p2start->value();
  }

  value.bit(16, 23) = dipSwitch[1];
  value.bit(24, 31) = dipSwitch[0];

  return value;
}

auto Aleck64::readPort2() -> u32 {
  n32 value = 0xffff'ffff;

  if(gameConfig->type() == BoardType::E92) {
    value.bit(16) = !aleck64.controls.p1start->value();
    value.bit(17) = !aleck64.controls.p2start->value();
    value.bit(18) = !aleck64.controls.p1coin->value();
    value.bit(19) = !aleck64.controls.p2coin->value();
    value.bit(20) = !aleck64.controls.service->value();
    value.bit(21) = !aleck64.controls.test->value();
  } else if(gameConfig->type() == BoardType::E90) {
    value.bit (0) = !aleck64.controls.p1coin->value();
    value.bit( 1) = !aleck64.controls.p2coin->value();
    value.bit( 4) = !aleck64.controls.service->value();
    value.bit( 5) = !aleck64.controls.test->value();
  }

  return value;
}

auto Aleck64::readPort3() -> u32 {
  return gameConfig->readExpansionPort();
}

auto Aleck64::writePort3(n32 data) -> void {
  return gameConfig->writeExpansionPort(data);
}

auto Aleck64::readPort4() -> u32 {
  debug(unimplemented, "[Aleck64::readPort4]");
  return 0x0;
}

auto Aleck64::writePort4(n32 data) -> void {
  debug(unimplemented, "[Aleck64::writePort3] ", hex(data, 8L));
}
