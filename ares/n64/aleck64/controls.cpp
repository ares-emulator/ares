auto Aleck64::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  service = node->append<Node::Input::Button>("Service");
  test    = node->append<Node::Input::Button>("Test");

  p1x     = node->append<Node::Input::Axis>("Player 1 X-Axis");
  p1y     = node->append<Node::Input::Axis>("Player 1 Y-Axis");
  p1up    = node->append<Node::Input::Button>("Player 1 Up");
  p1down  = node->append<Node::Input::Button>("Player 1 Down");
  p1left  = node->append<Node::Input::Button>("Player 1 Left");
  p1right = node->append<Node::Input::Button>("Player 1 Right");
  p1start = node->append<Node::Input::Button>("Player 1 Start");
  p1coin  = node->append<Node::Input::Button>("Player 1 Coin");

  for(auto n: range(9)) {
    string name = {"Player 1 Button ", 1 + n};
    p1[n] = node->append<Node::Input::Button>(name);
  }

  p2x     = node->append<Node::Input::Axis>("Player 2 X-Axis");
  p2y     = node->append<Node::Input::Axis>("Player 2 Y-Axis");
  p2up    = node->append<Node::Input::Button>("Player 2 Up");
  p2down  = node->append<Node::Input::Button>("Player 2 Down");
  p2left  = node->append<Node::Input::Button>("Player 2 Left");
  p2right = node->append<Node::Input::Button>("Player 2 Right");
  p2start = node->append<Node::Input::Button>("Player 2 Start");
  p2coin  = node->append<Node::Input::Button>("Player 2 Coin");

  for(auto n: range(9)) {
    string name = {"Player 2 Button ", 1 + n};
    p2[n] = node->append<Node::Input::Button>(name);
  }

  mahjongA     = node->append<Node::Input::Button>("Mahjong A");
  mahjongB     = node->append<Node::Input::Button>("Mahjong B");
  mahjongC     = node->append<Node::Input::Button>("Mahjong C");
  mahjongD     = node->append<Node::Input::Button>("Mahjong D");
  mahjongE     = node->append<Node::Input::Button>("Mahjong E");
  mahjongF     = node->append<Node::Input::Button>("Mahjong F");
  mahjongG     = node->append<Node::Input::Button>("Mahjong G");
  mahjongH     = node->append<Node::Input::Button>("Mahjong H");
  mahjongI     = node->append<Node::Input::Button>("Mahjong I");
  mahjongJ     = node->append<Node::Input::Button>("Mahjong J");
  mahjongK     = node->append<Node::Input::Button>("Mahjong K");
  mahjongL     = node->append<Node::Input::Button>("Mahjong L");
  mahjongM     = node->append<Node::Input::Button>("Mahjong M");
  mahjongN     = node->append<Node::Input::Button>("Mahjong N");
  mahjongKan   = node->append<Node::Input::Button>("Mahjong カン");
  mahjongPon   = node->append<Node::Input::Button>("Mahjong ポン");
  mahjongChi   = node->append<Node::Input::Button>("Mahjong チー");
  mahjongReach = node->append<Node::Input::Button>("Mahjong リーチ");
  mahjongRon   = node->append<Node::Input::Button>("Mahjong ロン");
}

auto Aleck64::Controls::poll() -> void {
  platform->input(service);
  platform->input(test);

  platform->input(p1x);
  platform->input(p1y);
  platform->input(p1up);
  platform->input(p1down);
  platform->input(p1left);
  platform->input(p1right);
  platform->input(p1start);
  platform->input(p1coin);

  for(auto n : range(9)) {
    platform->input(p1[n]);
  }

  platform->input(p2x);
  platform->input(p2y);
  platform->input(p2up);
  platform->input(p2down);
  platform->input(p2left);
  platform->input(p2right);
  platform->input(p2start);
  platform->input(p2coin);

  for(auto n : range(9)) {
    platform->input(p2[n]);
  }

  platform->input(mahjongA);
  platform->input(mahjongB);
  platform->input(mahjongC);
  platform->input(mahjongD);
  platform->input(mahjongE);
  platform->input(mahjongF);
  platform->input(mahjongG);
  platform->input(mahjongH);
  platform->input(mahjongI);
  platform->input(mahjongJ);
  platform->input(mahjongK);
  platform->input(mahjongL);
  platform->input(mahjongM);
  platform->input(mahjongN);
  platform->input(mahjongKan);
  platform->input(mahjongPon);
  platform->input(mahjongChi);
  platform->input(mahjongReach);
  platform->input(mahjongRon);
}

auto Aleck64::Controls::controllerButton(int playerIndex, string button) -> bool {
  if(!self.gameConfig) return 0;
  if(auto input = self.gameConfig->controllerButton(playerIndex, button)) {
    return input;
  }
  return 0;
}

auto Aleck64::Controls::controllerAxis(int playerIndex, string axis) -> s64 {
  if(!self.gameConfig) return 0;
  if(auto input = self.gameConfig->controllerAxis(playerIndex, axis)) {
    return input;
  }

  return 0;
}

auto Aleck64::Controls::ioPortControls(int port) -> n32 {
  if(!self.gameConfig) return 0xffff'ffff;
  if(auto input = self.gameConfig->ioPortControls(port)) {
    return input;
  }

  return 0xffff'ffff;
}

auto Aleck64::Controls::mahjong(n8 row) -> n8 {
  n8 value = 0xff;

  if(row.bit(0)) {
    value.bit(1) &= !aleck64.controls.mahjongB->value();
    value.bit(2) &= !aleck64.controls.mahjongF->value();
    value.bit(3) &= !aleck64.controls.mahjongJ->value();
    value.bit(4) &= !aleck64.controls.mahjongN->value();
    value.bit(5) &= !aleck64.controls.mahjongReach->value();
  }

  if(row.bit(1)) {
    value.bit(0) &= !aleck64.controls.p1start->value();
    value.bit(1) &= !aleck64.controls.mahjongA->value();
    value.bit(2) &= !aleck64.controls.mahjongE->value();
    value.bit(3) &= !aleck64.controls.mahjongI->value();
    value.bit(4) &= !aleck64.controls.mahjongM->value();
    value.bit(5) &= !aleck64.controls.mahjongKan->value();
  }

  if(row.bit(2)) {
    value.bit(1) &= !aleck64.controls.mahjongC->value();
    value.bit(2) &= !aleck64.controls.mahjongG->value();
    value.bit(3) &= !aleck64.controls.mahjongK->value();
    value.bit(4) &= !aleck64.controls.mahjongChi->value();
    value.bit(5) &= !aleck64.controls.mahjongRon->value();
  }

  if(row.bit(3)) {
    value.bit(1) &= !aleck64.controls.mahjongD->value();
    value.bit(2) &= !aleck64.controls.mahjongH->value();
    value.bit(3) &= !aleck64.controls.mahjongL->value();
    value.bit(4) &= !aleck64.controls.mahjongPon->value();
  }

  return value;
}