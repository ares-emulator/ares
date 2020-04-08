struct Expansion : Thread {
  Expansion();
  virtual ~Expansion();
  virtual auto main() -> void;
};

#include "port.hpp"
#include "satellaview/satellaview.hpp"
#include "21fx/21fx.hpp"
