#pragma once

namespace nall {

inline auto image::scale(uint outputWidth, uint outputHeight, bool linear) -> void {
  if(!_data) return;
  if(_width == outputWidth && _height == outputHeight) return;  //no scaling necessary
  if(linear == false) return scaleNearest(outputWidth, outputHeight);

  if(_width  == outputWidth ) return scaleLinearHeight(outputHeight);
  if(_height == outputHeight) return scaleLinearWidth(outputWidth);

  //find fastest scaling method, based on number of interpolation operations required
  //magnification usually benefits from two-pass linear interpolation
  //minification usually benefits from one-pass bilinear interpolation
  uint d1wh = ((_width  * outputWidth ) + (outputWidth * outputHeight)) * 1;
  uint d1hw = ((_height * outputHeight) + (outputWidth * outputHeight)) * 1;
  uint d2wh = (outputWidth * outputHeight) * 3;

  if(d1wh <= d1hw && d1wh <= d2wh) return scaleLinearWidth(outputWidth), scaleLinearHeight(outputHeight);
  if(d1hw <= d2wh) return scaleLinearHeight(outputHeight), scaleLinearWidth(outputWidth);
  return scaleLinear(outputWidth, outputHeight);
}

inline auto image::scaleLinearWidth(uint outputWidth) -> void {
  uint8_t* outputData = allocate(outputWidth, _height, stride());
  uint outputPitch = outputWidth * stride();
  uint64_t xstride = ((uint64_t)(_width - 1) << 32) / max(1u, outputWidth - 1);

  for(uint y = 0; y < _height; y++) {
    uint64_t xfraction = 0;

    const uint8_t* sp = _data + pitch() * y;
    uint8_t* dp = outputData + outputPitch * y;

    uint64_t a = read(sp);
    uint64_t b = read(sp + stride());
    sp += stride();

    uint x = 0;
    while(true) {
      while(xfraction < 0x100000000 && x++ < outputWidth) {
        write(dp, interpolate4i(a, b, xfraction));
        dp += stride();
        xfraction += xstride;
      }
      if(x >= outputWidth) break;

      sp += stride();
      a = b;
      b = read(sp);
      xfraction -= 0x100000000;
    }
  }

  free();
  _data = outputData;
  _width = outputWidth;
}

inline auto image::scaleLinearHeight(uint outputHeight) -> void {
  uint8_t* outputData = allocate(_width, outputHeight, stride());
  uint64_t ystride = ((uint64_t)(_height - 1) << 32) / max(1u, outputHeight - 1);

  for(uint x = 0; x < _width; x++) {
    uint64_t yfraction = 0;

    const uint8_t* sp = _data + stride() * x;
    uint8_t* dp = outputData + stride() * x;

    uint64_t a = read(sp);
    uint64_t b = read(sp + pitch());
    sp += pitch();

    uint y = 0;
    while(true) {
      while(yfraction < 0x100000000 && y++ < outputHeight) {
        write(dp, interpolate4i(a, b, yfraction));
        dp += pitch();
        yfraction += ystride;
      }
      if(y >= outputHeight) break;

      sp += pitch();
      a = b;
      b = read(sp);
      yfraction -= 0x100000000;
    }
  }

  free();
  _data = outputData;
  _height = outputHeight;
}

inline auto image::scaleLinear(uint outputWidth, uint outputHeight) -> void {
  uint8_t* outputData = allocate(outputWidth, outputHeight, stride());
  uint outputPitch = outputWidth * stride();

  uint64_t xstride = ((uint64_t)(_width  - 1) << 32) / max(1u, outputWidth  - 1);
  uint64_t ystride = ((uint64_t)(_height - 1) << 32) / max(1u, outputHeight - 1);

  for(uint y = 0; y < outputHeight; y++) {
    uint64_t yfraction = ystride * y;
    uint64_t xfraction = 0;

    const uint8_t* sp = _data + pitch() * (yfraction >> 32);
    uint8_t* dp = outputData + outputPitch * y;

    uint64_t a = read(sp);
    uint64_t b = read(sp + stride());
    uint64_t c = read(sp + pitch());
    uint64_t d = read(sp + pitch() + stride());
    sp += stride();

    uint x = 0;
    while(true) {
      while(xfraction < 0x100000000 && x++ < outputWidth) {
        write(dp, interpolate4i(a, b, c, d, xfraction, yfraction));
        dp += stride();
        xfraction += xstride;
      }
      if(x >= outputWidth) break;

      sp += stride();
      a = b;
      c = d;
      b = read(sp);
      d = read(sp + pitch());
      xfraction -= 0x100000000;
    }
  }

  free();
  _data = outputData;
  _width = outputWidth;
  _height = outputHeight;
}

inline auto image::scaleNearest(uint outputWidth, uint outputHeight) -> void {
  uint8_t* outputData = allocate(outputWidth, outputHeight, stride());
  uint outputPitch = outputWidth * stride();

  uint64_t xstride = ((uint64_t)_width  << 32) / outputWidth;
  uint64_t ystride = ((uint64_t)_height << 32) / outputHeight;

  for(uint y = 0; y < outputHeight; y++) {
    uint64_t yfraction = ystride * y;
    uint64_t xfraction = 0;

    const uint8_t* sp = _data + pitch() * (yfraction >> 32);
    uint8_t* dp = outputData + outputPitch * y;

    uint64_t a = read(sp);

    uint x = 0;
    while(true) {
      while(xfraction < 0x100000000 && x++ < outputWidth) {
        write(dp, a);
        dp += stride();
        xfraction += xstride;
      }
      if(x >= outputWidth) break;

      sp += stride();
      a = read(sp);
      xfraction -= 0x100000000;
    }
  }

  free();
  _data = outputData;
  _width = outputWidth;
  _height = outputHeight;
}

}
