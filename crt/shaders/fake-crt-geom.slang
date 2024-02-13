#version 450

// Simple scanlines with curvature and mask effects lifted from crt-geom
// original by hunterk, edited by DariusG

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
  float SCANLINE_SINE_COMP_B;
  float warpX;
  float warpY;
  float corner_round;
  float cgwg;
  float boost,monitor_gamma,crt_gamma;
  float SIZE,GLOW_LINE;

} params;

// Parameter lines go here:
#pragma parameter SCANLINE_SINE_COMP_B "Scanline Intensity" 0.3 0.0 1.0 0.05
#pragma parameter SIZE "Scanline size" 1.0 0.5 2.0 0.5
#pragma parameter warpX "warpX" 0.03 0.0 0.125 0.01
#pragma parameter warpY "warpY" 0.05 0.0 0.125 0.01
#pragma parameter corner_round "Corner Roundness" 0.030 0.005 0.100 0.005
#pragma parameter cgwg "CGWG mask brightness" 0.7 0.0 1.0 0.1
#pragma parameter crt_gamma "CRT Gamma" 2.4 1.0 4.0 0.05
#pragma parameter monitor_gamma "Monitor Gamma" 2.25 1.0 4.0 0.05
#pragma parameter boost "Bright boost " 0.20 0.00 1.00 0.02
#pragma parameter GLOW_LINE "Glowing line" 0.006 0.00 0.20 0.001


#define SCANLINE_SINE_COMP_B params.SCANLINE_SINE_COMP_B
#define SIZE params.SIZE
#define warpX params.warpX
#define warpY params.warpY
#define corner_round params.corner_round
#define cgwg params.cgwg
#define crt_gamma params.crt_gamma
#define monitor_gamma params.monitor_gamma
#define boost params.boost
#define GLOW_LINE params.GLOW_LINE


#define OutputSize params.OutputSize
#define SourceSize params.SourceSize
#define OriginalSize params.OriginalSize
#define FrameCount params.FrameCount
#define pi 3.141592
#define in_gamma  vec4(crt_gamma, crt_gamma, crt_gamma, 1.0)
#define out_gamma  vec4(1.0 / monitor_gamma, 1.0 / monitor_gamma, 1.0 / monitor_gamma, 1.0)
#define scale vec4(SourceSize.xy/OriginalSize.xy,OriginalSize.xy/SourceSize.xy)


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;

#define iTimer (float(FrameCount)*60.0)


vec4 scanline(vec2 coord, vec4 frame, float l)
{
    float SCANLINE_SINE_COMP = mix(SCANLINE_SINE_COMP_B*1.5,SCANLINE_SINE_COMP_B,l);
    vec3 res = frame.xyz;
    vec3 scanline = res * (SCANLINE_SINE_COMP*sin(fract(coord.y*SIZE*SourceSize.y)*pi)+1.0- SCANLINE_SINE_COMP);

    return vec4(scanline, 1.0);
}


// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos)
{
    pos  = pos*2.0-1.0;    
    pos *= vec2(1.0 + (pos.y*pos.y)*warpX, 1.0 + (pos.x*pos.x)*warpY);
    
    return pos*0.5 + 0.5;
}

float corner(vec2 coord)
{               

                coord *= scale.xy;
                coord = (coord - vec2(0.5)) * 1.0 + vec2(0.5);
                coord = min(coord, vec2(1.0)-coord) * vec2(1.0, OriginalSize.y/OriginalSize.x);
                vec2 cdist = vec2(corner_round);
                coord = (cdist - min(coord,cdist));
                float dist = sqrt(dot(coord,coord));
                return clamp((cdist.x-dist)*300.0,0.0, 1.0);
}  


// mask calculation
    // cgwg mask.
    vec4 Mask(vec2 pos)
    {
      vec3 mask = vec3(1.0);
    
      float m = fract(pos.x*0.5);
      if (m<0.5) return vec4(cgwg,cgwg,cgwg,1.0);
      else return vec4(mask,1.0);
 
    }

 float randomPass(vec2 coords)
 {
    return fract(smoothstep(-120.0, 0.0, coords.y - (SourceSize.y + 120.0) * fract(iTimer * 0.00015)));
}


void main()
{

    vec2 pos = Warp(vTexCoord.xy*(scale.xy))*scale.zw;
        vec2 OGL2Pos = pos * SourceSize.xy;
        vec2 pC4 = floor(OGL2Pos) + 0.5;
        vec2 coord = pC4 / SourceSize.xy;

        vec2 tc = vec2(pos.x, coord.y);

    vec4 res = texture(Source, tc);
    float l = max(max(res.r,res.g),res.b);
    res = scanline(pos,res,l);
    res = scanline(1.0-pos,res,l);
    res = scanline(1.0+pos,res,l);
    res = pow(res,in_gamma);

    // apply the mask; looks bad with vert scanlines so make them mutually exclusive
    res *= Mask(vTexCoord *OutputSize.xy * 1.0001);

#if defined GL_ES
    // hacky clamp fix for GLES
    vec2 bordertest = (tc);
    if ( bordertest.x > 0.0001 && bordertest.x < 0.9999 && bordertest.y > 0.0001 && bordertest.y < 0.9999)
        res = res;
    else
        res = vec4(0.,0.,0.,0.);
#endif
    vec4 color = res;

    // re-apply the gamma curve for the mask path
    color = pow(color, out_gamma);
    color += boost*color;
    color += randomPass(tc * SourceSize.xy) * GLOW_LINE;
    FragColor = color*corner(tc);

} 
