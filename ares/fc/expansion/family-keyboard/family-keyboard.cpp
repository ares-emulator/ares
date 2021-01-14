FamilyKeyboard::FamilyKeyboard(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Family Keyboard");

  key.f1 = node->append<Node::Input::Button>("F1");
  key.f2 = node->append<Node::Input::Button>("F2");
  key.f3 = node->append<Node::Input::Button>("F3");
  key.f4 = node->append<Node::Input::Button>("F4");
  key.f5 = node->append<Node::Input::Button>("F5");
  key.f6 = node->append<Node::Input::Button>("F6");
  key.f7 = node->append<Node::Input::Button>("F7");
  key.f8 = node->append<Node::Input::Button>("F8");

  key.one   = node->append<Node::Input::Button>("1");
  key.two   = node->append<Node::Input::Button>("2");
  key.three = node->append<Node::Input::Button>("3");
  key.four  = node->append<Node::Input::Button>("4");
  key.five  = node->append<Node::Input::Button>("5");
  key.six   = node->append<Node::Input::Button>("6");
  key.seven = node->append<Node::Input::Button>("7");
  key.eight = node->append<Node::Input::Button>("8");
  key.nine  = node->append<Node::Input::Button>("9");
  key.zero  = node->append<Node::Input::Button>("0");
  key.minus = node->append<Node::Input::Button>("Minus");
  key.power = node->append<Node::Input::Button>("^");
  key.yen   = node->append<Node::Input::Button>("Yen");
  key.stop  = node->append<Node::Input::Button>("Stop");

  key.escape = node->append<Node::Input::Button>("Escape");
  key.q      = node->append<Node::Input::Button>("Q");
  key.w      = node->append<Node::Input::Button>("W");
  key.e      = node->append<Node::Input::Button>("E");
  key.r      = node->append<Node::Input::Button>("R");
  key.t      = node->append<Node::Input::Button>("T");
  key.y      = node->append<Node::Input::Button>("Y");
  key.u      = node->append<Node::Input::Button>("U");
  key.i      = node->append<Node::Input::Button>("I");
  key.o      = node->append<Node::Input::Button>("O");
  key.p      = node->append<Node::Input::Button>("P");
  key.at     = node->append<Node::Input::Button>("@");
  key.lbrace = node->append<Node::Input::Button>("[");
  key.enter  = node->append<Node::Input::Button>("Return");

  key.control   = node->append<Node::Input::Button>("Control");
  key.a         = node->append<Node::Input::Button>("A");
  key.s         = node->append<Node::Input::Button>("S");
  key.d         = node->append<Node::Input::Button>("D");
  key.f         = node->append<Node::Input::Button>("F");
  key.g         = node->append<Node::Input::Button>("G");
  key.h         = node->append<Node::Input::Button>("H");
  key.j         = node->append<Node::Input::Button>("J");
  key.k         = node->append<Node::Input::Button>("K");
  key.l         = node->append<Node::Input::Button>("L");
  key.semicolon = node->append<Node::Input::Button>(";");
  key.colon     = node->append<Node::Input::Button>(":");
  key.rbrace    = node->append<Node::Input::Button>("]");
  key.kana      = node->append<Node::Input::Button>("Kana");

  key.lshift     = node->append<Node::Input::Button>("Left Shift");
  key.z          = node->append<Node::Input::Button>("Z");
  key.x          = node->append<Node::Input::Button>("X");
  key.c          = node->append<Node::Input::Button>("C");
  key.v          = node->append<Node::Input::Button>("V");
  key.b          = node->append<Node::Input::Button>("B");
  key.n          = node->append<Node::Input::Button>("N");
  key.m          = node->append<Node::Input::Button>("M");
  key.comma      = node->append<Node::Input::Button>(",");
  key.period     = node->append<Node::Input::Button>(".");
  key.slash      = node->append<Node::Input::Button>("/");
  key.underscore = node->append<Node::Input::Button>("_");
  key.rshift     = node->append<Node::Input::Button>("Right Shift");

  key.graph    = node->append<Node::Input::Button>("Graph");
  key.spacebar = node->append<Node::Input::Button>("Spacebar");

  key.home      = node->append<Node::Input::Button>("Home");
  key.insert    = node->append<Node::Input::Button>("Insert");
  key.backspace = node->append<Node::Input::Button>("Delete");

  key.up    = node->append<Node::Input::Button>("Up");
  key.down  = node->append<Node::Input::Button>("Down");
  key.left  = node->append<Node::Input::Button>("Left");
  key.right = node->append<Node::Input::Button>("Right");
}

auto FamilyKeyboard::read1() -> n1 {
  n1 data;
  //data recorder (unsupported)
  return data;
}

auto FamilyKeyboard::read2() -> n5 {
  if(!latch.bit(2)) return 0b00000;

  #define poll(id, name) \
    platform->input(key.name); \
    data.bit(id) = !key.name->value();
  n5 data;
  switch(row << 1 | column) {
  case  0:
    poll(1, f8);
    poll(2, enter);
    poll(3, lbrace);
    poll(4, rbrace);
    break;
  case  1:
    poll(1, kana);
    poll(2, rshift);
    poll(3, yen);
    poll(4, stop);
    break;
  case  2:
    poll(1, f7);
    poll(2, at);
    poll(3, colon);
    poll(4, semicolon);
    break;
  case  3:
    poll(1, underscore);
    poll(2, slash);
    poll(3, minus);
    poll(4, power);
    break;
  case  4:
    poll(1, f6);
    poll(2, o);
    poll(3, l);
    poll(4, k);
    break;
  case  5:
    poll(1, period);
    poll(2, comma);
    poll(3, p);
    poll(4, zero);
    break;
  case  6:
    poll(1, f5);
    poll(2, i);
    poll(3, u);
    poll(4, j);
    break;
  case  7:
    poll(1, m);
    poll(2, n);
    poll(3, nine);
    poll(4, eight);
    break;
  case  8:
    poll(1, f4);
    poll(2, y);
    poll(3, g);
    poll(4, h);
    break;
  case  9:
    poll(1, b);
    poll(2, v);
    poll(3, seven);
    poll(4, six);
    break;
  case 10:
    poll(1, f3);
    poll(2, t);
    poll(3, r);
    poll(4, d);
    break;
  case 11:
    poll(1, f);
    poll(2, c);
    poll(3, five);
    poll(4, four);
    break;
  case 12:
    poll(1, f2);
    poll(2, w);
    poll(3, s);
    poll(4, a);
    break;
  case 13:
    poll(1, x);
    poll(2, z);
    poll(3, e);
    poll(4, three);
    break;
  case 14:
    poll(1, f1);
    poll(2, escape);
    poll(3, q);
    poll(4, control);
    break;
  case 15:
    poll(1, lshift);
    poll(2, graph);
    poll(3, one);
    poll(4, two);
    break;
  case 16:
    poll(1, home);
    poll(2, up);
    poll(3, right);
    poll(4, left);
    break;
  case 17:
    poll(1, down);
    poll(2, spacebar);
    poll(3, backspace);
    poll(4, insert);
    break;
  case 18:  //device detection ID
  case 19:  //device detection ID
    data = 0x1e;
    break;
  }
  #undef poll

  return data;
}

auto FamilyKeyboard::write(n3 data) -> void {
  latch = data;
  if(column && !latch.bit(1)) row = (row + 1) % 10;
  column = latch.bit(1);
  if(latch.bit(0)) row = 0;
}
