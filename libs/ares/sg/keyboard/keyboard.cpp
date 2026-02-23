#include <sg/sg.hpp>

namespace ares::SG1000 {

Keyboard keyboard;

auto Keyboard::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Keyboard");

  // Define the keyboard matrix
  string labels[7][12] = {
      {"1", "Q", "A", "Z", "ED" , "," , "K", "I", "8"  , ""   , ""   , ""   },
      {"2", "W", "S", "X", "SPC", "." , "L", "O", "9"  , ""   , ""   , ""   },
      {"3", "E", "D", "C", "HC" , "/" , ";", "P", "0"  , ""   , ""   , ""   },
      {"4", "R", "F", "V", "ID" , "PI", ":", "@", "-"  , ""   , ""   , ""   },
      {"5", "T", "G", "B", ""   , "DA", "]", "[", "^"  , ""   , ""   , ""   },
      {"6", "Y", "H", "N", ""   , "LA", "CR", "", "YEN", ""   , ""   , "FNC"},
      {"7", "U", "J", "M", ""   , "RA", "UA", "", "BRK", "GRP", "CTL", "SHF"},
  };

  for(u32 column : range(12)) {
    for(u32 row : range(7)) {
      if(labels[row][column] != "") {
        matrix[row][column] = node->append<Node::Input::Button>(labels[row][column]);
      }
    }
  }
}

auto Keyboard::unload() -> void {
  node.reset();
}

auto Keyboard::read(n3 row) -> n12 {
  n12 output = 0xff;

  for(auto col : range(12))
  {
    if(auto node= matrix[row][col]) {
      platform->input(node);
      output.bit(col) = !node->value();
    }
  }

  return output;
}

}
