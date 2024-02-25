///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file contains all the stuff used to make these shaders compile both as GLSL and HLSL.
//
// Note that one of GLSL or HLSL must be defined.


#ifndef NTSC_UTIL_LANG
  #define NTSC_UTIL_LANG

  #ifndef GLSL
    #ifndef HLSL
      // Decfine this to make my syntax highlighting less angry
      #define HLSL
    #endif
  #endif
  #ifdef GLSL
    #define uint2 uvec2
    #define uint3 uvec3
    #define uint4 uvec4
    #define int2 ivec2
    #define int3 ivec3
    #define int4 ivec4
    #define float2 vec2
    #define float3 vec3
    #define float4 vec4
    #define ddx dFdx
    #define ddy dFdy
    #define frac fract
    #define lerp mix

    precision mediump float;

    float saturate(float a)
    {
      return clamp(a, 0.0, 1.0);
    }

    float2 saturate(float2 a)
    {
      return clamp(a, 0.0, 1.0);
    }

    float3 saturate(float3 a)
    {
      return clamp(a, 0.0, 1.0);
    }

    float4 saturate(float4 a)
    {
      return clamp(a, 0.0, 1.0);
    }

    void sincos(float angle, out float s, out float c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    void sincos(float2 angle, out float2 s, out float2 c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    void sincos(float3 angle, out float3 s, out float3 c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    void sincos(float4 angle, out float4 s, out float4 c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    #define CONST const
    #define BEGIN_CONST_ARRAY(type, name, size) const type name[size] = type[size](
    #define END_CONST_ARRAY );

    // GLSL doesn't have separate samplers and textures, so we're using texName for everything
    #define DECLARE_TEXTURE2D(texName, samplerName) uniform sampler2D texName

    #define DECLARE_TEXTURE2D_AND_SAMPLER_PARAM(texName, samplerName) sampler2D texName
    #define PASS_TEXTURE2D_AND_SAMPLER_PARAM(texName, samplerName) texName
    #define SAMPLE_TEXTURE(texName, samplerName, coord) texture2D(texName, vec2((coord).x, 1.0 - (coord).y))
    #define SAMPLE_TEXTURE_BIAS(texName, samplerName, coord, bias) \
      texture2D(texName, vec2((coord).x, 1.0 - (coord).y), bias)
    #define GET_TEXTURE_SIZE(tex, outVar) outVar = textureSize(tex, 0)

    #define PS_MAIN in float2 vsOutTexCoord; out float4 psOutPos; void main() { psOutPos = Main(vsOutTexCoord); }

    #define CBUFFER uniform
  #endif

  #ifdef HLSL
    #define CONST static const

    #define BEGIN_CONST_ARRAY(type, name, size) static const type name[size] = {
    #define END_CONST_ARRAY };

    #define DECLARE_TEXTURE2D(texName, samplerName) Texture2D<float4> texName; sampler samplerName

    #define DECLARE_TEXTURE2D_AND_SAMPLER_PARAM(texName, samplerName) Texture2D<float4> texName, sampler samplerName
    #define PASS_TEXTURE2D_AND_SAMPLER_PARAM(texName, samplerName) texName, samplerName
    #define SAMPLE_TEXTURE(texName, samplerName, coord) texName.Sample(samplerName, coord)
    #define SAMPLE_TEXTURE_BIAS(texName, samplerName, coord, bias) texName.SampleBias(samplerName, coord, bias)
    #define GET_TEXTURE_SIZE(tex, outVar) tex.GetDimensions(outVar.x, outVar.y)

    #define PS_MAIN float4 main(float2 inTexCoord: TEX): SV_TARGET { return Main(inTexCoord); }

    #define CBUFFER cbuffer
  #endif
#endif