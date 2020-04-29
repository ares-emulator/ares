#include <msx/msx.hpp>

namespace ares::MSX {

Keyboard keyboard;
#include "serialization.cpp"

auto Keyboard::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>("Keyboard");
  port->setFamily("MSX");
  port->setType("Keyboard");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(port, name); });
  port->setConnect([&] { connect(); });
  port->setDisconnect([&] { disconnect(); });
  port->setSupported({"Japanese"});
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
  if(auto fp = platform->open(layout, "layout.bml", File::Read)) {
    document = BML::unserialize(fp->reads());
  }
  for(uint column : range(12)) {
    for(uint row : range(8)) {
      string label{column, ",", row};
      if(auto key = document[{"layout/key[", column * 8 + row, "]"}]) {
        label = key.text();
      }
      matrix[column][row] = layout->append<Node::Button>(label);
    }
  }
}

auto Keyboard::disconnect() -> void {
  layout = {};
  for(uint column : range(12)) {
    for(uint row : range(8)) {
      matrix[column][row] = {};
    }
  }
}

auto Keyboard::power() -> void {
  io = {};
}

auto Keyboard::read() -> uint8 {
  uint8 column = io.select;
  uint8 data = 0xff;
  if(column >= 0 && column <= 11) {
    for(uint row : range(8)) {
      if(auto node = matrix[column][row]) {
        platform->input(node);
        data.bit(row) = !node->value();
      }
    }
  }
  return data;
}

auto Keyboard::write(uint4 data) -> void {
  io.select = data;
}

}
