#define lerp mix

#define NTSC 0
#define PAL 1

// Effect params
#define VIDEO_STANDARD PAL

#if VIDEO_STANDARD == NTSC
    const vec2 maxResLuminance = vec2(333.0, 480.0);
    const vec2 maxResChroma = vec2(40.0, 480.0);
#elif VIDEO_STANDARD == PAL
    const vec2 maxResLuminance = vec2(335.0, 576.0);
    const vec2 maxResChroma = vec2(40.0, 240.0);
#endif

const vec2 blurAmount = vec2(0.2, 0.2);

// End effect params

#define VIDEO_TEXTURE iChannel0



mat3 rgb2yiq = mat3(0.299, 0.596, 0.211,
                        0.587, -0.274, -0.523,
                        0.114, -0.322, 0.312);

mat3 yiq2rgb = mat3(1, 1, 1,
                        0.956, -0.272, -1.106,
                        0.621, -0.647, 1.703);

// from http://www.java-gaming.org/index.php?topic=35123.0
vec4 cubic(float v)
{
    vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return vec4(x, y, z, w) * (1.0/6.0);
}

vec4 textureBicubic(sampler2D tex, vec2 texCoords)
{

    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 invTexSize = vec2(1.0) / texSize;

    texCoords = texCoords * texSize - 0.5;


    vec2 fxy = fract(texCoords);
    texCoords -= fxy;

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = texCoords.xxyy + vec2 (-0.5, +1.5).xyxy;

    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4 (xcubic.yw, ycubic.yw) / s;

    offset *= invTexSize.xxyy;

    vec4 sample0 = texture(tex, offset.xz);
    vec4 sample1 = texture(tex, offset.yz);
    vec4 sample2 = texture(tex, offset.xw);
    vec4 sample3 = texture(tex, offset.yw);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(
       mix(sample3, sample2, sx), mix(sample1, sample0, sx)
    , sy);
}