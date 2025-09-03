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

inline auto merge(std::initializer_list<string> range, string_view separator = "") -> string {
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

inline auto isplit(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  auto tmp = vector<string>()._split<1, 0>(source, on, limit);
  std::vector<string> out;
  out.reserve(tmp.size());
  for(u32 i = 0; i < tmp.size(); i++) out.push_back(tmp[i]);
  return out;
}

inline auto qsplit(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  auto tmp = vector<string>()._split<0, 1>(source, on, limit);
  std::vector<string> out;
  out.reserve(tmp.size());
  for(u32 i = 0; i < tmp.size(); i++) out.push_back(tmp[i]);
  return out;
}

inline auto iqsplit(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  auto tmp = vector<string>()._split<1, 1>(source, on, limit);
  std::vector<string> out;
  out.reserve(tmp.size());
  for(u32 i = 0; i < tmp.size(); i++) out.push_back(tmp[i]);
  return out;
}

// Helper function to replace vector<string>::match
inline auto match(const vector<string>& strings, string_view pattern) -> std::vector<string> {
  std::vector<string> result;
  for(u32 n = 0; n < strings.size(); n++) {
    if(strings[n].match(pattern)) result.push_back(strings[n]);
  }
  return result;
}

// Helper function to strip a single string (for use with ranges)
inline auto strip(const string& s) -> string {
  auto copy = s;
  return copy.strip();
}

// Helper function that combines split + strip for convenience
inline auto split_and_strip(string_view source, string_view delimiter, long limit = LONG_MAX) -> std::vector<string> {
  auto tmp = split(source, delimiter, limit);
  std::vector<string> result;
  result.reserve(tmp.size());
  for(const auto& item : tmp | std::views::transform(strip)) {
    result.push_back(item);
  }
  return result;
}

}


