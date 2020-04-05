#pragma once

namespace nall {

inline image::image(const image& source) {
  operator=(source);
}

inline image::image(image&& source) {
  operator=(forward<image>(source));
}

inline image::image(bool endian, uint depth, uint64_t alphaMask, uint64_t redMask, uint64_t greenMask, uint64_t blueMask) {
  _endian = endian;
  _depth  = depth;

  _alpha = {alphaMask, bitDepth(alphaMask), bitShift(alphaMask)};
  _red   = {redMask,   bitDepth(redMask),   bitShift(redMask  )};
  _green = {greenMask, bitDepth(greenMask), bitShift(greenMask)};
  _blue  = {blueMask,  bitDepth(blueMask),  bitShift(blueMask )};
}

inline image::image(const string& filename) {
  load(filename);
}

inline image::image(const void* data_, uint size) {
  auto data = (const uint8_t*)data_;
  if(size < 4);
  else if(data[0] == 'B' && data[1] == 'M') loadBMP(data, size);
  else if(data[1] == 'P' && data[2] == 'N' && data[3] == 'G') loadPNG(data, size);
}

inline image::image(const vector<uint8_t>& buffer) : image(buffer.data(), buffer.size()) {
}

template<uint Size> inline image::image(const uint8_t (&Name)[Size]) : image(Name, Size) {
}

inline image::image() {
}

inline image::~image() {
  free();
}

inline auto image::operator=(const image& source) -> image& {
  if(this == &source) return *this;
  free();

  _width = source._width;
  _height = source._height;

  _endian = source._endian;
  _depth = source._depth;

  _alpha = source._alpha;
  _red = source._red;
  _green = source._green;
  _blue = source._blue;

  _data = allocate(_width, _height, stride());
  memory::copy(_data, source._data, source.size());
  return *this;
}

inline auto image::operator=(image&& source) -> image& {
  if(this == &source) return *this;
  free();

  _width = source._width;
  _height = source._height;

  _endian = source._endian;
  _depth = source._depth;

  _alpha = source._alpha;
  _red = source._red;
  _green = source._green;
  _blue = source._blue;

  _data = source._data;
  source._data = nullptr;
  return *this;
}

inline image::operator bool() const {
  return _data && _width && _height;
}

inline auto image::operator==(const image& source) const -> bool {
  if(_width != source._width) return false;
  if(_height != source._height) return false;

  if(_endian != source._endian) return false;
  if(_depth != source._depth) return false;

  if(_alpha != source._alpha) return false;
  if(_red != source._red) return false;
  if(_green != source._green) return false;
  if(_blue != source._blue) return false;

  return memory::compare(_data, source._data, size()) == 0;
}

inline auto image::operator!=(const image& source) const -> bool {
  return !operator==(source);
}

inline auto image::read(const uint8_t* data) const -> uint64_t {
  uint64_t result = 0;
  if(_endian == 0) {
    for(int n = stride() - 1; n >= 0; n--) result = (result << 8) | data[n];
  } else {
    for(int n = 0; n < stride(); n++) result = (result << 8) | data[n];
  }
  return result;
}

inline auto image::write(uint8_t* data, uint64_t value) const -> void {
  if(_endian == 0) {
    for(int n = 0; n < stride(); n++) {
      data[n] = value;
      value >>= 8;
    }
  } else {
    for(int n = stride() - 1; n >= 0; n--) {
      data[n] = value;
      value >>= 8;
    }
  }
}

inline auto image::free() -> void {
  if(_data) delete[] _data;
  _data = nullptr;
}

inline auto image::load(const string& filename) -> bool {
  if(loadBMP(filename) == true) return true;
  if(loadPNG(filename) == true) return true;
  return false;
}

//assumes image and data are in the same format; pitch is adapted to image
inline auto image::copy(const void* data, uint pitch, uint width, uint height) -> void {
  allocate(width, height);
  for(uint y : range(height)) {
    auto input = (const uint8_t*)data + y * pitch;
    auto output = (uint8_t*)_data + y * this->pitch();
    memory::copy(output, input, width * stride());
  }
}

inline auto image::allocate(uint width, uint height) -> void {
  if(_data && _width == width && _height == height) return;
  free();
  _width = width;
  _height = height;
  _data = allocate(_width, _height, stride());
}

//private
inline auto image::allocate(uint width, uint height, uint stride) -> uint8_t* {
  //allocate 1x1 larger than requested; so that linear interpolation does not require bounds-checking
  uint size = width * height * stride;
  uint padding = width * stride + stride;
  auto data = new uint8_t[size + padding];
  memory::fill(data + size, padding);
  return data;
}

}
