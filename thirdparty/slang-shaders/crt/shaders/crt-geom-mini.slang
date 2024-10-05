#version 450


// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.


layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
  float CURV;
  float scanlines;
  float MASK;
  float INTERL;
  float SAT;
} params;

// Parameter lines go here:

#pragma parameter CURV "CRT-Geom Curvature" 1.0 0.0 1.0 1.0
#pragma parameter scanlines "CRT-Geom Scanline Weight" 0.5 0.0 0.5 0.05
#pragma parameter MASK "CRT-Geom Dotmask Strength" 0.2 0.0 0.5 0.05
#pragma parameter INTERL "CRT-Geom Interlacing Simulation" 1.0 0.0 1.0 1.0
#pragma parameter SAT "CRT-Geom Saturation" 1.0 0.0 2.0 0.05

#define PI 3.1415926535897932384626433
#define tau 6.2831852

#define SourceSize params.SourceSize
#define OriginalSize params.OriginalSize
#define OutputSize params.OutputSize
#define FrameCount params.FrameCount

#define CURV params.CURV
#define scanlines params.scanlines
#define MASK params.MASK
#define INTERL params.INTERL
#define SAT params.SAT

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float maskpos;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord * 1.0001;
    maskpos = vTexCoord.x*OutputSize.x*PI;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(location = 1) in float maskpos;

layout(set = 0, binding = 1) uniform sampler2D Source;


// Configuration.

float kaizer_x (float p)
{
    // Compute sinc filter.
    float k = sin(1.3* ((p - 1.0) / 2.0));
    return k;
}

vec2 Warp(vec2 coord)
{
        vec2 CURVATURE_DISTORTION = vec2(0.12, 0.25);
        // Barrel distortion shrinks the display area a bit, this will allow us to counteract that.
        vec2 barrelScale = vec2(0.97,0.945);
        coord -= vec2(0.5);
        float rsq = coord.x*coord.x + coord.y*coord.y;
        coord += coord * (CURVATURE_DISTORTION * rsq);
        coord *= barrelScale;
        if (abs(coord.x) >= 0.5 || abs(coord.y) >= 0.5)
                coord = vec2(-1.0);             // If out of bounds, return an invalid value.
        else
        {
                coord += vec2(0.5);
        }

        return coord;
}
void main()
{
vec3 res = vec3(0.0);
vec2 dx = vec2(SourceSize.z*0.5,0.0); //sharpness
vec2 pos, corn;
if(CURV == 1.0){
 pos = Warp(vTexCoord);
 corn = min(pos, 1.0-pos);    // This is used to mask the rounded
     corn.x = 0.0001/corn.x;      // corners later on
}
else pos = vTexCoord;

vec2 xy = pos;
xy -= dx*2.0;
vec2 near = floor(pos*SourceSize.xy)+0.5;
vec2 f = pos*SourceSize.xy - near;

xy.y = (near.y + 16.0*f.y*f.y*f.y*f.y*f.y)*SourceSize.w;    

//kaizer precalculated
res += texture(Source,xy).rgb*-1.6;
res += texture(Source,xy+2.0*dx).rgb*3.3;
res += texture(Source,xy+3.0*dx).rgb*5.6;
res += texture(Source,xy+4.0*dx).rgb*-1.5;
    
res /= 5.8;
    float a = dot(vec3(0.25),res);
    float s = mix(scanlines,scanlines*0.6,a);

    float texsize = 1.0;
    float fp = 0.25;
    if (OriginalSize.y > 400.0) texsize = 0.5;

    if (INTERL == 1.0 && OriginalSize.y > 400.0) 
    {
    fp = mod(float(FrameCount),2.0) <1.0 ? 0.5+fp :fp;
    }


    float scan = s*sin((pos.y*SourceSize.y*texsize-fp)*tau)+1.0-s;
    float mask = MASK*sin(maskpos)+1.0-MASK;
    res *= scan*mask;
    res *= mix(1.45,1.05,a);
    float l = dot(vec3(0.29, 0.6, 0.11), res);
    res  = mix(vec3(l), res, SAT);
    res = clamp(res,0.0,1.0);
    if (corn.y <= corn.x && CURV == 1.0 || corn.x < 0.0001 && CURV ==1.0 )res = vec3(0.0);
    FragColor.rgb = sqrt(res);
}
