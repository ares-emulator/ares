#pragma once

namespace nall {

inline auto string::boolean() const -> bool {
  return equals("true");
}

inline auto string::integer() const -> intmax {
  return toInteger(data());
}

inline auto string::natural() const -> uintmax {
  return toNatural(data());
}

inline auto string::hex() const -> uintmax {
  return toHex(data());
}

inline auto string::real() const -> double {
  return toReal(data());
}

}
