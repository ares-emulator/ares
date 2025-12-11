#pragma once

#include <optional>
#include <ranges>
#include <vector>

#include <nall/string.hpp>

namespace nall {

namespace internal {
  template<bool Insensitive, bool Quoted>
  inline auto _split(string_view source, string_view find, long limit) -> std::vector<string> { 
    std::vector<string> out;
    if(limit <= 0 || find.size() == 0) return out;

    const char* p = source.data();
    s32 size = source.size();
    s32 base = 0;
    s32 matches = 0;

    for(s32 n = 0, quoted = 0; n <= size - (s32)find.size();) {
      if constexpr(Quoted) {
        if(quoted && p[n] == '\\') { n += 2; continue; }
        if(p[n] == '\'' && quoted != 2) { quoted ^= 1; n++; continue; }
        if(p[n] == '\"' && quoted != 1) { quoted ^= 2; n++; continue; }
        if(quoted) { n++; continue; }
      }
      if(string::_compare<Insensitive>(p + n, size - n, find.data(), find.size())) { n++; continue; }
      if(matches >= limit) break;

      out.emplace_back(string_view(p + base, (u32)(n - base)));
      n += find.size();
      base = n;
      matches++;
    }

    out.emplace_back(string_view(p + base, (u32)(size - base)));
    return out;
  }
}

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
  return internal::_split<false, false>(source, on, limit);
}

inline auto isplit(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  return internal::_split<true, false>(source, on, limit);
}

inline auto qsplit(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  return internal::_split<false, true>(source, on, limit);
}

inline auto iqsplit(string_view source, string_view on, long limit = LONG_MAX) -> std::vector<string> {
  return internal::_split<true, true>(source, on, limit);
}

// Helper function to replace vector<string>::match
inline auto match(const std::vector<string>& strings, string_view pattern) -> std::vector<string> {
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


