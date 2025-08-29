#pragma once

#include <optional>
#include <ranges>

namespace nall {

template <std::ranges::range R, class T>
inline auto index_of(const R& range, const T& value) -> std::optional<size_t> {
  if (auto it = std::ranges::find(range, value); it != std::ranges::end(range)) {
    return static_cast<size_t>(std::ranges::distance(std::ranges::begin(range), it));
  }
  return std::nullopt;
}

}


