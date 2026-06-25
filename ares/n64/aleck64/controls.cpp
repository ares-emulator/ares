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
}

auto Aleck64::Controls::controllerButton(int playerIndex, string button) -> bool {
  if(playerIndex == 1) {
    if(button == "Up"     ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p1up->value();
    if(button == "Down"   ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p1down->value();
    if(button == "Left"   ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p1left->value();
    if(button == "Right"  ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p1right->value();
    if(button == "Start"  ) return aleck64.controls.p1start->value();
    if(button == "A"      ) return aleck64.controls.p1[0]->value();
    if(button == "B"      ) return aleck64.controls.p1[1]->value();
    if(button == "R"      ) return aleck64.controls.p1[2]->value();
    if(button == "C-Right") return aleck64.controls.p1[3]->value();
  }

  if(playerIndex == 2) {
    if(button == "Up"     ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p2up->value();
    if(button == "Down"   ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p2down->value();
    if(button == "Left"   ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p2left->value();
    if(button == "Right"  ) return aleck64.gameConfig->dpadDisabled() ? 1 : aleck64.controls.p2right->value();
    if(button == "Start"  ) return aleck64.controls.p2start->value();
    if(button == "A"      ) return aleck64.controls.p2[0]->value();
    if(button == "B"      ) return aleck64.controls.p2[1]->value();
    if(button == "R"      ) return aleck64.controls.p2[2]->value();
    if(button == "C-Right") return aleck64.controls.p2[3]->value();
  }

  return 0;
}

auto Aleck64::Controls::controllerAxis(int playerIndex, string axis) -> s64 {
  if(playerIndex == 1) {
    if(axis == "X") return aleck64.controls.p1x->value();
    if(axis == "Y") return aleck64.controls.p1y->value();
  }

  if(playerIndex == 2) {
    if(axis == "X") return aleck64.controls.p2x->value();
    if(axis == "Y") return aleck64.controls.p2y->value();
  }

  return 0;
}

auto Aleck64::Mahjong::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Mahjong");

  a     = node->append<Node::Input::Button>("A");
  b     = node->append<Node::Input::Button>("B");
  c     = node->append<Node::Input::Button>("C");
  d     = node->append<Node::Input::Button>("D");
  e     = node->append<Node::Input::Button>("E");
  f     = node->append<Node::Input::Button>("F");
  g     = node->append<Node::Input::Button>("G");
  h     = node->append<Node::Input::Button>("H");
  i     = node->append<Node::Input::Button>("I");
  j     = node->append<Node::Input::Button>("J");
  k     = node->append<Node::Input::Button>("K");
  l     = node->append<Node::Input::Button>("L");
  m     = node->append<Node::Input::Button>("M");
  n     = node->append<Node::Input::Button>("N");
  kan   = node->append<Node::Input::Button>("カン");
  pon   = node->append<Node::Input::Button>("ポン");
  chi   = node->append<Node::Input::Button>("チー");
  reach = node->append<Node::Input::Button>("リーチ");
  ron   = node->append<Node::Input::Button>("ロン");
}

auto Aleck64::Mahjong::poll() -> void {
  platform->input(a);
  platform->input(b);
  platform->input(c);
  platform->input(d);
  platform->input(e);
  platform->input(f);
  platform->input(g);
  platform->input(h);
  platform->input(i);
  platform->input(j);
  platform->input(k);
  platform->input(l);
  platform->input(m);
  platform->input(n);
  platform->input(kan);
  platform->input(pon);
  platform->input(chi);
  platform->input(reach);
  platform->input(ron);
}

auto Aleck64::Mahjong::read(n8 row) -> n8 {
  n8 value = 0xff;

  if(row.bit(0)) {
    value.bit(1) &= !b->value();
    value.bit(2) &= !f->value();
    value.bit(3) &= !j->value();
    value.bit(4) &= !n->value();
    value.bit(5) &= !reach->value();
  }

  if(row.bit(1)) {
    value.bit(0) &= !aleck64.controls.p1start->value();
    value.bit(1) &= !a->value();
    value.bit(2) &= !e->value();
    value.bit(3) &= !i->value();
    value.bit(4) &= !m->value();
    value.bit(5) &= !kan->value();
  }

  if(row.bit(2)) {
    value.bit(1) &= !c->value();
    value.bit(2) &= !g->value();
    value.bit(3) &= !k->value();
    value.bit(4) &= !chi->value();
    value.bit(5) &= !ron->value();
  }

  if(row.bit(3)) {
    value.bit(1) &= !d->value();
    value.bit(2) &= !h->value();
    value.bit(3) &= !l->value();
    value.bit(4) &= !pon->value();
  }

  return value;
}
