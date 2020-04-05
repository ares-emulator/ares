#pragma once

#include <algorithm>

#include <nall/file-map.hpp>
#include <nall/interpolation.hpp>
#include <nall/stdint.hpp>
#include <nall/decode/bmp.hpp>
#include <nall/decode/png.hpp>

namespace nall {

struct image {
  enum class blend : uint {
    add,
    sourceAlpha,  //color = sourceColor * sourceAlpha + targetColor * (1 - sourceAlpha)
    sourceColor,  //color = sourceColor
    targetAlpha,  //color = targetColor * targetAlpha + sourceColor * (1 - targetAlpha)
    targetColor,  //color = targetColor
  };

  struct channel {
    channel(uint64_t mask, uint depth, uint shift) : _mask(mask), _depth(depth), _shift(shift) {
    }

    auto operator==(const channel& source) const -> bool {
      return _mask == source._mask && _depth == source._depth && _shift == source._shift;
    }

    auto operator!=(const channel& source) const -> bool {
      return !operator==(source);
    }

    auto mask() const { return _mask; }
    auto depth() const { return _depth; }
    auto shift() const { return _shift; }

  private:
    uint64_t _mask;
    uint _depth;
    uint _shift;
  };

  //core.hpp
  image(const image& source);
  image(image&& source);
  image(bool endian, uint depth, uint64_t alphaMask, uint64_t redMask, uint64_t greenMask, uint64_t blueMask);
  image(const string& filename);
  image(const void* data, uint size);
  image(const vector<uint8_t>& buffer);
  template<uint Size> image(const uint8_t (&Name)[Size]);
  image();
  ~image();

  auto operator=(const image& source) -> image&;
  auto operator=(image&& source) -> image&;

  explicit operator bool() const;
  auto operator==(const image& source) const -> bool;
  auto operator!=(const image& source) const -> bool;

  auto read(const uint8_t* data) const -> uint64_t;
  auto write(uint8_t* data, uint64_t value) const -> void;

  auto free() -> void;
  auto load(const string& filename) -> bool;
  auto copy(const void* data, uint pitch, uint width, uint height) -> void;
  auto allocate(uint width, uint height) -> void;

  //fill.hpp
  auto fill(uint64_t color = 0) -> void;
  auto gradient(uint64_t a, uint64_t b, uint64_t c, uint64_t d) -> void;
  auto gradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY, function<double (double, double)> callback) -> void;
  auto crossGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;
  auto diamondGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;
  auto horizontalGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;
  auto radialGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;
  auto sphericalGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;
  auto squareGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;
  auto verticalGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void;

  //scale.hpp
  auto scale(uint width, uint height, bool linear = true) -> void;

  //blend.hpp
  auto impose(blend mode, uint targetX, uint targetY, image source, uint x, uint y, uint width, uint height) -> void;

  //utility.hpp
  auto shrink(uint64_t transparentColor = 0) -> void;
  auto crop(uint x, uint y, uint width, uint height) -> bool;
  auto alphaBlend(uint64_t alphaColor) -> void;
  auto alphaMultiply() -> void;
  auto transform(const image& source = {}) -> void;
  auto transform(bool endian, uint depth, uint64_t alphaMask, uint64_t redMask, uint64_t greenMask, uint64_t blueMask) -> void;

  //static.hpp
  static auto bitDepth(uint64_t color) -> uint;
  static auto bitShift(uint64_t color) -> uint;
  static auto normalize(uint64_t color, uint sourceDepth, uint targetDepth) -> uint64_t;

  //access
  auto data() { return _data; }
  auto data() const { return _data; }
  auto width() const { return _width; }
  auto height() const { return _height; }

  auto endian() const { return _endian; }
  auto depth() const { return _depth; }
  auto stride() const { return (_depth + 7) >> 3; }

  auto pitch() const { return _width * stride(); }
  auto size() const { return _height * pitch(); }

  auto alpha() const { return _alpha; }
  auto red() const { return _red; }
  auto green() const { return _green; }
  auto blue() const { return _blue; }

private:
  //core.hpp
  auto allocate(uint width, uint height, uint stride) -> uint8_t*;

  //scale.hpp
  auto scaleLinearWidth(uint width) -> void;
  auto scaleLinearHeight(uint height) -> void;
  auto scaleLinear(uint width, uint height) -> void;
  auto scaleNearest(uint width, uint height) -> void;

  //load.hpp
  auto loadBMP(const string& filename) -> bool;
  auto loadBMP(const uint8_t* data, uint size) -> bool;
  auto loadPNG(const string& filename) -> bool;
  auto loadPNG(const uint8_t* data, uint size) -> bool;

  //interpolation.hpp
  auto isplit(uint64_t* component, uint64_t color) -> void;
  auto imerge(const uint64_t* component) -> uint64_t;
  auto interpolate1f(uint64_t a, uint64_t b, double x) -> uint64_t;
  auto interpolate1f(uint64_t a, uint64_t b, uint64_t c, uint64_t d, double x, double y) -> uint64_t;
  auto interpolate1i(int64_t a, int64_t b, uint32_t x) -> uint64_t;
  auto interpolate1i(int64_t a, int64_t b, int64_t c, int64_t d, uint32_t x, uint32_t y) -> uint64_t;
  auto interpolate4f(uint64_t a, uint64_t b, double x) -> uint64_t;
  auto interpolate4f(uint64_t a, uint64_t b, uint64_t c, uint64_t d, double x, double y) -> uint64_t;
  auto interpolate4i(uint64_t a, uint64_t b, uint32_t x) -> uint64_t;
  auto interpolate4i(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint32_t x, uint32_t y) -> uint64_t;

  uint8_t* _data   = nullptr;
  uint _width  = 0;
  uint _height = 0;

  bool _endian =  0;  //0 = lsb, 1 = msb
  uint _depth  = 32;

  channel _alpha{255u << 24, 8, 24};
  channel _red  {255u << 16, 8, 16};
  channel _green{255u <<  8, 8,  8};
  channel _blue {255u <<  0, 8,  0};
};

}

#include <nall/image/static.hpp>
#include <nall/image/core.hpp>
#include <nall/image/load.hpp>
#include <nall/image/interpolation.hpp>
#include <nall/image/fill.hpp>
#include <nall/image/scale.hpp>
#include <nall/image/blend.hpp>
#include <nall/image/utility.hpp>
