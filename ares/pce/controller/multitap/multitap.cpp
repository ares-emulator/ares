Multitap::Multitap(Node::Port parent):
port1{"Controller Port 1"},
port2{"Controller Port 2"},
port3{"Controller Port 3"},
port4{"Controller Port 4"},
port5{"Controller Port 5"}
{
  node = parent->append<Node::Peripheral>("Multitap");

  port1.load(node);
  port2.load(node);
  port3.load(node);
  port4.load(node);
  port5.load(node);
}

auto Multitap::read() -> n4 {
  if (counter == 0) return port1.read();
  if (counter == 1) return port2.read();
  if (counter == 2) return port3.read();
  if (counter == 3) return port4.read();
  if (counter == 4) return port5.read();

  // When counter > num ports, multi-tap returns 0
  return 0;
}

auto Multitap::write(n2 data) -> void {
  auto prevSel = sel;
  auto prevClr = clr;

  sel = data.bit(0);
  clr = data.bit(1);

  // Counter is incremented on 0-1 transition of sel
  // but only while clr is 0
  if (!clr && sel && !prevSel) {
    counter++;
  } 

  // Counter is reset on 0-1 transition of clr
  // but only while sel is 1
  if (sel && clr && !prevClr) {
    counter = 0;
  }

  if (counter == 0) return port1.write(sel);
  if (counter == 1) return port2.write(sel);
  if (counter == 2) return port3.write(sel);
  if (counter == 3) return port4.write(sel);
  if (counter == 4) return port5.write(sel);
}
