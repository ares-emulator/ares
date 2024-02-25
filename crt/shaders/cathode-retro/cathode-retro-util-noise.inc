#ifndef noise_func
  #define noise_func


  float Noise2D(vec2 coord, float iseed)
  {
    float fseed = fract(iseed / 10000.0);
    float angle = fract(distance(coord.xy, 1000.0 * (vec2(fseed + 0.3, 0.1) + 1.0)));
    return fract(tan(angle) * distance(coord.xy, 1000.0 * (vec2(fseed + 0.1, fseed + 0.2) - 2.0)));
  }


  float Noise1D(float coord, float iseed)
  {
    return Noise2D(vec2(coord, 0.0), iseed);
  }
#endif
