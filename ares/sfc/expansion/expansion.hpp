struct Expansion : Thread {
  Expansion();
  virtual ~Expansion();
  virtual auto main() -> void;
};

#include <sfc/expansion/port.hpp>
#include <sfc/expansion/satellaview/satellaview.hpp>
#include <sfc/expansion/21fx/21fx.hpp>
