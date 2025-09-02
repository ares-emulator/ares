#include "extension.hpp"
using namespace nall;

#include <nall/serializer.hpp>
#include <ranges>

namespace hiro {
  #include "../resource/resource.cpp"
  #include "../resource/icon.cpp"
  #include "fixed-layout.cpp"
  #include "horizontal-layout.cpp"
  #include "vertical-layout.cpp"
  #include "table-layout.cpp"
  #include "horizontal-resize-grip.cpp"
  #include "vertical-resize-grip.cpp"
  #include "list-view.cpp"
  #include "message-dialog.cpp"
  #include "name-dialog.cpp"
  #include "browser-dialog.cpp"
  #include "about-dialog.cpp"
}
