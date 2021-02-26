#version 150

//anti-aliased nearest-neighbor

precision highp float;

uniform sampler2D source[];
uniform vec4 sourceSize[];
uniform vec4 targetSize[];

in Vertex {
  vec2 texCoord;
};

out vec4 fragColor;

vec4 vpow(vec4 n, float e) {
  return vec4(pow(n.x, e), pow(n.y, e), pow(n.z, e), pow(n.w, e));
}

vec4 toLQV(vec3 c) {
  return vec4(c.r, c.g, c.b, c.r * 0.2989 + c.g * 0.5870 + c.b * 0.1140);
}

vec3 fromLQV(vec4 c) {
  float f = c.w / (c.r * 0.2989 + c.g * 0.5870 + c.b * 0.1140);
  return vec3(c.rgb) * f;
}

vec3 percent(float ssize, float tsize, float coord) {
  float minfull  = (coord * tsize - 0.5) / tsize * ssize;
  float maxfull  = (coord * tsize + 0.5) / tsize * ssize;
  float realfull = floor(maxfull);

  if(minfull > realfull) {
    return vec3(
      1,
      (realfull + 0.5) / ssize,
      (realfull + 0.5) / ssize
    );
  }

  return vec3(
    (maxfull - realfull) / (maxfull - minfull),
    (realfull - 0.5) / ssize,
    (realfull + 0.5) / ssize
  );
}

void main() {
  float srgb  = 2.1;
  float gamma = 3.0;

  vec3 x = percent(sourceSize[0].x, targetSize[0].x, texCoord.x);
  vec3 y = percent(sourceSize[0].y, targetSize[0].y, texCoord.y);

  //get points to interpolate across in linear RGB
  vec4 a = toLQV(vpow(texture(source[0], vec2(x[1], y[1])), srgb).rgb);
  vec4 b = toLQV(vpow(texture(source[0], vec2(x[2], y[1])), srgb).rgb);
  vec4 c = toLQV(vpow(texture(source[0], vec2(x[1], y[2])), srgb).rgb);
  vec4 d = toLQV(vpow(texture(source[0], vec2(x[2], y[2])), srgb).rgb);

  //use perceptual gamma for luminance component
  a.w = pow(a.w, 1 / gamma);
  b.w = pow(b.w, 1 / gamma);
  c.w = pow(c.w, 1 / gamma);
  d.w = pow(d.w, 1 / gamma);

  //interpolate
  vec4 gammaLQV =
    (1.0 - x[0]) * (1.0 - y[0]) * a +
    (0.0 + x[0]) * (1.0 - y[0]) * b +
    (1.0 - x[0]) * (0.0 + y[0]) * c +
    (0.0 + x[0]) * (0.0 + y[0]) * d;

  //convert luminance gamma back to linear
  gammaLQV.w = pow(gammaLQV.w, gamma);

  //convert color back to sRGB
  fragColor = vpow(vec4(fromLQV(gammaLQV), 1), 1 / srgb);
}
