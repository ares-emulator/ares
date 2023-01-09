#include <spec/spec.hpp>

namespace ares::ZXSpectrum {

Keyboard keyboard;

auto Keyboard::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>("Keyboard");
  port->setFamily("ZX Spectrum");
  port->setType("Keyboard");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(port, name); });
  port->setConnect([&] { connect(); });
  port->setDisconnect([&] { disconnect(); });
  port->setSupported({"Original"});
}

auto Keyboard::unload() -> void {
  disconnect();
  port = {};
}

auto Keyboard::allocate(Node::Port parent, string name) -> Node::Peripheral {
  return layout = parent->append<Node::Peripheral>(name);
}

auto Keyboard::connect() -> void {
  Markup::Node document;

  // Define the keyboard matrix
  string labels[8][5] = {
    { "CAPS SHIFT", "Z", "X", "C," "V" },
    { "A", "S", "D", "F", "G" },
    { "Q", "W", "E", "R", "T" },
    { "1", "2", "3", "4", "5" },
    { "0", "9", "8", "7", "6" },
    { "P", "O", "I", "U", "Y" },
    { "ENTER", "L", "K", "J", "H" },
    { "SPACE BREAK", "SYMBOL SHIFT", "M", "N", "B" },
  };

  for(u32 column : range(5)) {
    for(u32 row : range(8)) {
      matrix[row][column] = layout->append<Node::Input::Button>(labels[row][column]);
    }
  }
}

auto Keyboard::disconnect() -> void {
  layout = {};
  for(u32 column : range(5)) {
    for(u32 row : range(8)) {
      matrix[row][column] = {};
    }
  }
}

auto Keyboard::power() -> void {

}

auto Keyboard::read(u8 row) -> n5 {
  n5 data = 0x1f;

  for(uint _key : range(5)) {
    if(auto node = matrix[row][_key]) {
      platform->input(node);
      data.bit(_key) = !node->value();
    }
  }

  return data;
}

}
