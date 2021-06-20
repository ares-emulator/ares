auto CPU::Keypad::read() -> n4 {
  n4 data;
  bool horizontal = ppu.screen->rotation() == 0;

  if(Model::WonderSwan() || Model::WonderSwanColor() || Model::SwanCrystal()) {
    if(matrix.bit(0)) {  //d4
      if(horizontal) {
        data.bit(0) = system.controls.y1->value();
        data.bit(1) = system.controls.y2->value();
        data.bit(2) = system.controls.y3->value();
        data.bit(3) = system.controls.y4->value();
      } else {
        data.bit(0) = system.controls.x4->value();
        data.bit(1) = system.controls.x1->value();
        data.bit(2) = system.controls.x2->value();
        data.bit(3) = system.controls.x3->value();
      }
    }

    if(matrix.bit(1)) {  //d5
      if(horizontal) {
        data.bit(0) = system.controls.x1->value();
        data.bit(1) = system.controls.x2->value();
        data.bit(2) = system.controls.x3->value();
        data.bit(3) = system.controls.x4->value();
      } else {
        data.bit(0) = system.controls.y4->value();
        data.bit(1) = system.controls.y1->value();
        data.bit(2) = system.controls.y2->value();
        data.bit(3) = system.controls.y3->value();
      }
    }

    if(matrix.bit(2)) {  //d6
      data.bit(1) = system.controls.start->value();
      data.bit(2) = system.controls.a->value();
      data.bit(3) = system.controls.b->value();
    }
  }

  if(Model::PocketChallengeV2()) {
    //this pin is always forced to logic high, which has the practical effect of bypassing the IPLROM.
    data.bit(1) = 1;

    if(matrix.bit(0)) {  //d4
      data.bit(0) = system.controls.clear->value();
      data.bit(2) = system.controls.circle->value();
      data.bit(3) = system.controls.pass->value();
    }

    if(matrix.bit(1)) {  //d5
      data.bit(0) = system.controls.view->value();
      data.bit(2) = system.controls.escape->value();
      data.bit(3) = system.controls.rightLatch;
    }

    if(matrix.bit(2)) {  //d6
      data.bit(0) = system.controls.leftLatch;
      data.bit(2) = system.controls.down->value();
      data.bit(3) = system.controls.up->value();
    }
  }

  return data;
}
