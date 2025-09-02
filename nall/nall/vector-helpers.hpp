#pragma once

#include <optional>
#include <ranges>
#include <vector>

#include <nall/string.hpp>

namespace nall {

template <std::ranges::range R, class T>
inline auto index_of(const R& range, const T& value) -> std::optional<size_t> {
  if (auto it = std::ranges::find(range, value); it != std::ranges::end(range)) {
    return static_cast<size_t>(std::ranges::distance(std::ranges::begin(range), it));
  }
  return std::nullopt;
}

template<typename Range>
inline auto merge(const Range& range, string_view separator = "") -> string {
  string output;
  bool first = true;
  for(const auto& item : range) {
    if(!first) output.append(separator);
    first = false;
    output.append(item);
  }
  return output;
}

inline auto split(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  auto tmp = vector<string>()._split<0, 0>(source, on, limit);
  std::vector<string> out;
  out.reserve(tmp.size());
  for(u32 i = 0; i < tmp.size(); i++) out.push_back(tmp[i]);
  return out;
}

}


