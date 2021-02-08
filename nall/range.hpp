#pragma once

namespace nall {

struct range_t {
  struct iterator {
    iterator(s64 position, s64 step = 0) : position(position), step(step) {}
    auto operator*() const -> s64 { return position; }
    auto operator!=(const iterator& source) const -> bool { return step > 0 ? position < source.position : position > source.position; }
    auto operator++() -> iterator& { position += step; return *this; }

  private:
    s64 position;
    const s64 step;
  };

  struct reverse_iterator {
    reverse_iterator(s64 position, s64 step = 0) : position(position), step(step) {}
    auto operator*() const -> s64 { return position; }
    auto operator!=(const reverse_iterator& source) const -> bool { return step > 0 ? position > source.position : position < source.position; }
    auto operator++() -> reverse_iterator& { position -= step; return *this; }

  private:
    s64 position;
    const s64 step;
  };

  auto begin() const -> iterator { return {origin, stride}; }
  auto end() const -> iterator { return {target}; }

  auto rbegin() const -> reverse_iterator { return {target - stride, stride}; }
  auto rend() const -> reverse_iterator { return {origin - stride}; }

  s64 origin;
  s64 target;
  s64 stride;
};

inline auto range(s64 size) {
  return range_t{0, size, 1};
}

inline auto range(s64 offset, s64 size) {
  return range_t{offset, size, 1};
}

inline auto range(s64 offset, s64 size, s64 step) {
  return range_t{offset, size, step};
}

}
