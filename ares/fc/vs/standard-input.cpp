VsUniSystem::Controls::StandardInput::StandardInput(Node::Object parent, Wiring wiring, n8 fixedBits)
  : wiring(wiring), fixedBits(fixedBits) {
  loadPlayer(parent, players[0], 1);
  loadPlayer(parent, players[1], 2);

  if(wiring == Wiring::Standard) live[0] = players[1].a, live[1] = players[0].a;
  if(wiring == Wiring::Swapped)  live[0] = players[0].a, live[1] = players[1].a;
  if(wiring == Wiring::SwapAB)   live[0] = players[1].b, live[1] = players[0].b;
}

auto VsUniSystem::Controls::StandardInput::loadPlayer(Node::Object parent, Player& player, u32 id) -> void {
  auto prefix = string{"Player ", id, " "};

  player.up    = parent->append<Node::Input::Button>(string{prefix, "Up"      });
  player.down  = parent->append<Node::Input::Button>(string{prefix, "Down"    });
  player.left  = parent->append<Node::Input::Button>(string{prefix, "Left"    });
  player.right = parent->append<Node::Input::Button>(string{prefix, "Right"   });
  player.a     = parent->append<Node::Input::Button>(string{prefix, "Button 1"});
  player.b     = parent->append<Node::Input::Button>(string{prefix, "Button 2"});
  player.start = parent->append<Node::Input::Button>(string{prefix, "Start"   });
}

auto VsUniSystem::Controls::StandardInput::pollPlayer(u32 player) -> n8 {
  auto& controls = players[player];
  platform->input(controls.a);
  platform->input(controls.b);
  platform->input(controls.start);
  platform->input(controls.up);
  platform->input(controls.down);
  platform->input(controls.left);
  platform->input(controls.right);

  n8 data = fixedBits;
  data.bit(0) = controls.a->value();
  data.bit(1) = controls.b->value();
  data.bit(2) = controls.start->value();
  data.bit(4) = controls.up->value();
  data.bit(5) = controls.down->value();
  data.bit(6) = controls.left->value();
  data.bit(7) = controls.right->value();
  return data;
}

auto VsUniSystem::Controls::StandardInput::data(u32 stream) -> n1 {
  platform->input(live[stream]);
  return live[stream]->value();
}

auto VsUniSystem::Controls::StandardInput::latch() -> std::array<n8, 2> {
  auto player1 = pollPlayer(0);
  auto player2 = pollPlayer(1);
  if(wiring == Wiring::Swapped) return {player1, player2};

  player2.bit(2) = players[0].start->value();
  player1.bit(2) = players[1].start->value();
  if(wiring == Wiring::SwapAB) {
    n1 player1A = player1.bit(0);
    n1 player2A = player2.bit(0);
    player1.bit(0) = player1.bit(1);
    player1.bit(1) = player1A;
    player2.bit(0) = player2.bit(1);
    player2.bit(1) = player2A;
  }
  return {player2, player1};
}
