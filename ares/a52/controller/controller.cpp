#include <a52/a52.hpp>

namespace ares::Atari5200 {

Controller::Controller(Node::Port parent, string name) {
  node = parent->append<Node::Peripheral>(name);
}

auto Controller::appendControls() -> void {
  top    = node->append<Node::Input::Button>("Top Fire");
  bottom = node->append<Node::Input::Button>("Bottom Fire");
  one    = node->append<Node::Input::Button>("1");
  two    = node->append<Node::Input::Button>("2");
  three  = node->append<Node::Input::Button>("3");
  four   = node->append<Node::Input::Button>("4");
  five   = node->append<Node::Input::Button>("5");
  six    = node->append<Node::Input::Button>("6");
  seven  = node->append<Node::Input::Button>("7");
  eight  = node->append<Node::Input::Button>("8");
  nine   = node->append<Node::Input::Button>("9");
  star   = node->append<Node::Input::Button>("*");
  zero   = node->append<Node::Input::Button>("0");
  pound  = node->append<Node::Input::Button>("#");
  start  = node->append<Node::Input::Button>("Start");
  pause  = node->append<Node::Input::Button>("Pause");
  reset  = node->append<Node::Input::Button>("Reset");
}

auto Controller::poll() -> void {
  bottomFireLevel = pressed(bottom);
}

auto Controller::pressed(Node::Input::Button input) -> bool {
  if(platform) platform->input(input);
  return input->value();
}

auto Controller::keypad(n4 code) -> bool {
  switch(code) {
  case 0x0f: return pressed(one);
  case 0x0e: return pressed(two);
  case 0x0d: return pressed(three);
  case 0x0c: return pressed(start);
  case 0x0b: return pressed(four);
  case 0x0a: return pressed(five);
  case 0x09: return pressed(six);
  case 0x08: return pressed(pause);
  case 0x07: return pressed(seven);
  case 0x06: return pressed(eight);
  case 0x05: return pressed(nine);
  case 0x04: return pressed(reset);
  case 0x03: return pressed(star);
  case 0x02: return pressed(zero);
  case 0x01: return pressed(pound);
  }
  return false;
}

auto Controller::topFire() -> bool {
  return pressed(top);
}

auto Controller::bottomFire() -> bool {
  return bottomFireLevel;
}

#include "port.cpp"
#include "standard/standard.cpp"
#include "trak-ball/trak-ball.cpp"

}
