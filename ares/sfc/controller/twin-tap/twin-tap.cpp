TwinTap::TwinTap(Node::Port parent, Node::Peripheral with) {
  node = Node::append<Node::Peripheral>(parent, with, "Twin Tap");

  one = Node::append<Node::Button>(node, with, "1");
  two = Node::append<Node::Button>(node, with, "2");
}

auto TwinTap::data() -> uint2 {
  if(latched == 1) {
    platform->input(two);
    return two->value();  //unconfirmed
  }

  switch(counter++) {
  case 0: return two->value();
  case 1: return one->value();
  }
  if(counter < 24) return 0;  //unconfirmed
  counter = 24;  //unconfirmed
  return 1;  //unconfirmed
}

//unconfirmed
auto TwinTap::latch(bool data) -> void {
  if(latched == data) return;
  latched = data;
  counter = 0;

  if(latched == 0) {
    platform->input(two);
    platform->input(one);
  }
}
