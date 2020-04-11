TwinTap::TwinTap(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Twin Tap");

  one = node->append<Node::Button>("1");
  two = node->append<Node::Button>("2");
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
