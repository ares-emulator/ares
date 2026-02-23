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
//if(auto fp = platform->open(layout, "layout.bml", File::Read)) {
//  document = BML::unserialize(fp->reads());
//}


  // Define the keyboard matrix
  string labels[12][8] = {
    { "0 わ を" , "1 ! ぬ", "2 \" ふ", "3 # あ ぁ", "4 $ う ぅ", "5 % え ぇ", "6 & お ぉ", "7 ’ や ゃ" },
    { "8 ( ゆ ゅ", "9 ) よ ょ", "- = ほ", "^ ~ へ", "¥ | ー", "@ ‘ \"", "[ { 。", "; + れ" },
    { ": * け", "] } む", ", < ね `", ". > る 。", "/ ? め .", "- ろ", "A ち", "B こ" },
    { "C そ", "D し", "E い ぃ", "F は", "G き", "H く", "I に", "J ま" },
    { "K の", "L り", "M も", "N み", "O ら", "P せ", "Q た", "R す" },
    { "S と", "T か", "U な", "V ひ", "W て", "X さ", "Y ん", "Z つ っ" },
    { "SHIFT", "CTRL", "GRAPH", "CAPS", "かな", "F1 F6", "F2 F7", "F3 F8" },
    { "F4 F9", "F5 F10", "ESC", "TAB", "STOP", "BS", "SELECT", "RETURN" },
    { "SPACE", "CLS/HOME", "INS", "DEL", "←", "↑", "↓", "→" }, 
    { "*", "+", "/", "0", "1", "2", "3", "4", },
    { "5", "6", "7", "8", "9", "-", ",", ".", },
    { "",  "実行", "", "取消", "", "", "", "" }
  };

  for(u32 column : range(12)) {
    for(u32 row : range(8)) {
      matrix[column][row] = layout->append<Node::Input::Button>(labels[column][row]);
    }
  }
}

auto Keyboard::disconnect() -> void {
  layout = {};
  for(u32 column : range(12)) {
    for(u32 row : range(8)) {
      matrix[column][row] = {};
    }
  }
}

auto Keyboard::power() -> void {
  io = {};
}

auto Keyboard::read() -> n8 {
  n8 column = io.select;
  n8 data = 0xff;
  if(column >= 0 && column <= 11) {
    for(u32 row : range(8)) {
      if(auto node = matrix[column][row]) {
        platform->input(node);
        data.bit(row) = !node->value();
      }
    }
  }
  return data;
}

auto Keyboard::write(n4 data) -> void {
  io.select = data;
}

}
