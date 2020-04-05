#pragma once

namespace nall {

inline auto image::fill(uint64_t color) -> void {
  for(uint y = 0; y < _height; y++) {
    uint8_t* dp = _data + pitch() * y;
    for(uint x = 0; x < _width; x++) {
      write(dp, color);
      dp += stride();
    }
  }
}

inline auto image::gradient(uint64_t a, uint64_t b, uint64_t c, uint64_t d) -> void {
  for(uint y = 0; y < _height; y++) {
    uint8_t* dp = _data + pitch() * y;
    double muY = (double)y / (double)_height;
    for(uint x = 0; x < _width; x++) {
      double muX = (double)x / (double)_width;
      write(dp, interpolate4f(a, b, c, d, muX, muY));
      dp += stride();
    }
  }
}

inline auto image::gradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY, function<double (double, double)> callback) -> void {
  for(int y = 0; y < _height; y++) {
    uint8_t* dp = _data + pitch() * y;
    double py = max(-radiusY, min(+radiusY, y - centerY)) * 1.0 / radiusY;
    for(int x = 0; x < _width; x++) {
      double px = max(-radiusX, min(+radiusX, x - centerX)) * 1.0 / radiusX;
      double mu = max(0.0, min(1.0, callback(px, py)));
      if(mu != mu) mu = 1.0;  //NaN
      write(dp, interpolate4f(a, b, mu));
      dp += stride();
    }
  }
}

inline auto image::crossGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    x = fabs(x), y = fabs(y);
    return min(x, y) * min(x, y);
  });
}

inline auto image::diamondGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    return fabs(x) + fabs(y);
  });
}

inline auto image::horizontalGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    return fabs(x);
  });
}

inline auto image::radialGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    return sqrt(x * x + y * y);
  });
}

inline auto image::sphericalGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    return x * x + y * y;
  });
}

inline auto image::squareGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    return max(fabs(x), fabs(y));
  });
}

inline auto image::verticalGradient(uint64_t a, uint64_t b, int radiusX, int radiusY, int centerX, int centerY) -> void {
  return gradient(a, b, radiusX, radiusY, centerX, centerY, [](double x, double y) -> double {
    return fabs(y);
  });
}

}
