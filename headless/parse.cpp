#include "parse.hpp"
#include <limits>

using namespace nall;

namespace headless {

auto parsePositiveInteger(const string& value, u64& out) -> bool {
  if(value.length() == 0) return false;
  u64 parsed = 0;
  for(auto ch : value) {
    if(ch < '0' || ch > '9') return false;
    u64 digit = ch - '0';
    if(parsed > (std::numeric_limits<u64>::max() - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  if(parsed == 0) return false;
  out = parsed;
  return true;
}

}
