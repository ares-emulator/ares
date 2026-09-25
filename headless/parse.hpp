#pragma once

#include <nall/string.hpp>

namespace headless {

auto parsePositiveInteger(const nall::string& value, u64& out) -> bool;

}
