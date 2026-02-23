auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  B  = node->append<Node::Input::Button>("UP [B]");
  C  = node->append<Node::Input::Button>("DOWN [C]");
  A  = node->append<Node::Input::Button>("LEFT [A]");
  D  = node->append<Node::Input::Button>("RIGHT [D]");
  E  = node->append<Node::Input::Button>("ACTION [E]");
  
  _1  = node->append<Node::Input::Button>("1");
  _2  = node->append<Node::Input::Button>("2");
  _3  = node->append<Node::Input::Button>("3");
  _4  = node->append<Node::Input::Button>("4");
  _5  = node->append<Node::Input::Button>("5");
  _6  = node->append<Node::Input::Button>("6");
  _7  = node->append<Node::Input::Button>("7");
  _8  = node->append<Node::Input::Button>("8");
  _9  = node->append<Node::Input::Button>("9");
  _10 = node->append<Node::Input::Button>("10");
  _11 = node->append<Node::Input::Button>("11");
  _12 = node->append<Node::Input::Button>("12");
  _13 = node->append<Node::Input::Button>("13");
  _14 = node->append<Node::Input::Button>("14");
}

auto System::Controls::poll() -> void {
  
  platform->input(B);
  platform->input(C);
  platform->input(A);
  platform->input(D);
  platform->input(E);
  
  platform->input(_1);
  platform->input(_2);
  platform->input(_3);
  platform->input(_4);
  platform->input(_5);
  platform->input(_6);
  platform->input(_7);
  platform->input(_8);
  platform->input(_9);
  platform->input(_10);
  platform->input(_11);
  platform->input(_12);
  platform->input(_13);
  platform->input(_14);
}
