#version 450

// Original bits by Themaister
// Moire mitigation bits by Timothy Lottes, added by hunterk
 
layout(push_constant) uniform Push
{
   float BLOOM_STRENGTH;
   float OUTPUT_GAMMA;
   float CURVATURE;
   float warpX;
   float warpY;
   float shadowMask;
   float maskDark;
   float maskLight;
   float cornersize;
   float cornersmooth;
   float noise_amt;
} params;
 
#pragma parameter BLOOM_STRENGTH "Glow Strength" 0.45 0.0 0.8 0.05
#pragma parameter OUTPUT_GAMMA "Monitor Gamma" 2.2 1.8 2.6 0.02
#pragma parameter CURVATURE "Curvature" 0.0 0.0 1.0 1.0
#pragma parameter warpX "Curvature X-Axis" 0.031 0.0 0.125 0.01
#pragma parameter warpY "Curvature Y-Axis" 0.041 0.0 0.125 0.01
vec2 Distortion = vec2(params.warpX, params.warpY) * 15.;
#pragma parameter cornersize "Corner Size" 0.01 0.001 1.0 0.005
#define cornersize params.cornersize
#pragma parameter cornersmooth "Corner Smoothness" 1000.0 80.0 2000.0 100.0
#define cornersmooth params.cornersmooth
#pragma parameter noise_amt "Noise Amount" 1.0 0.0 5.0 0.25
#pragma parameter shadowMask "Mask Effect" 0.0 0.0 4.0 1.0
#pragma parameter maskDark "maskDark" 0.5 0.0 2.0 0.1
#pragma parameter maskLight "maskLight" 1.5 0.0 2.0 0.1
 
#define iTime mod(float(global.FrameCount) / 60.0, 600.0)
#define fragCoord (vTexCoord.xy * global.OutputSize.xy)
 
layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    vec4 CRTPassSize;
   uint FrameCount;
} global;
 
#pragma stage vertex
layout(location = 0) in  vec4  Position;
layout(location = 1) in  vec2  TexCoord;
layout(location = 0) out vec2  vTexCoord;
 
void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
}
 
#pragma stage fragment
layout(location = 0) in  vec2  vTexCoord;
layout(location = 0) out vec4  FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;
layout(set = 0, binding = 2) uniform sampler2D CRTPass;
 
// For debugging
#define BLOOM_ONLY 0
 
#define CRT_PASS CRTPass
 
// Convert from linear to sRGB.
//float Srgb(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}
vec4 Srgb(vec4 c){return pow(c, vec4(1.0 / 2.2));}
 
// Convert from sRGB to linear.
//float Linear(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}
float Linear(float c){return pow(c, 2.2);}
 
float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p){
	vec2 ip = floor(p);
	vec2 u = fract(p);
	u = u*u*(3.0-2.0*u);
	
	float res = mix(
		mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
		mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
	return res*res;
}

const vec2 corner_aspect   = vec2(1.0,  0.75);

float corner(vec2 coord)
{
    coord = (coord - vec2(0.5)) + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0) - coord) * corner_aspect;
    vec2 cdist = vec2(cornersize);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    
    return clamp((cdist.x - dist)*cornersmooth, 0.0, 1.0);
}

vec2 Warp(vec2 texCoord){
  float offset_x = noise(sin(gl_FragCoord.xy) * float(mod(global.FrameCount, 361.)));
  float offset_y = noise(cos(gl_FragCoord.yx) * float(mod(global.FrameCount, 873.)));
  vec2 noisecoord = texCoord + vec2(offset_x, offset_y) * 0.001 * params.noise_amt;
  vec2 curvedCoords = noisecoord * 2.0 - 1.0;
  float curvedCoordsDistance = sqrt(curvedCoords.x*curvedCoords.x+curvedCoords.y*curvedCoords.y);

  curvedCoords = curvedCoords / curvedCoordsDistance;

  curvedCoords = curvedCoords * (1.0-pow(vec2(1.0-(curvedCoordsDistance/1.4142135623730950488016887242097)),(1.0/(1.0+Distortion*0.2))));

  curvedCoords = curvedCoords / (1.0-pow(vec2(0.29289321881345247559915563789515),(1.0/(vec2(1.0)+Distortion*0.2))));

  curvedCoords = curvedCoords * 0.5 + 0.5;
  return curvedCoords;
}
 
// Shadow mask.
vec3 Mask(vec2 pos)
{
    vec3 mask = vec3(params.maskDark, params.maskDark, params.maskDark);
 
    // Very compressed TV style shadow mask.
    if (params.shadowMask == 1.0)
    {
        float line = params.maskLight;
        float odd = 0.0;
       
        if (fract(pos.x*0.166666666) < 0.5) odd = 1.0;
        if (fract((pos.y + odd) * 0.5) < 0.5) line = params.maskDark;  
       
        pos.x = fract(pos.x*0.333333333);
 
        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
        mask*=line;  
    }
 
    // Aperture-grille.
    else if (params.shadowMask == 2.0)
    {
        pos.x = fract(pos.x*0.333333333);
 
        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
    }
 
    // Stretched VGA style shadow mask (same as prior shaders).
    else if (params.shadowMask == 3.0)
    {
        pos.x += pos.y*3.0;
        pos.x  = fract(pos.x*0.166666666);
 
        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
    }
 
    // VGA style shadow mask.
    else if (params.shadowMask == 4.0)
    {
        pos.xy  = floor(pos.xy*vec2(1.0, 0.5));
        pos.x  += pos.y*3.0;
        pos.x   = fract(pos.x*0.166666666);
 
        if      (pos.x < 0.333) mask.r = params.maskLight;
        else if (pos.x < 0.666) mask.g = params.maskLight;
        else                    mask.b = params.maskLight;
    }
 
    return mask;
}
 
void main()
{
   vec2 pp = vTexCoord.xy;
   pp = (params.CURVATURE > 0.5) ? Warp(pp) : pp;
   
#if BLOOM_ONLY
    vec3 source = BLOOM_STRENGTH * texture(Source, pp).rgb;
#else
 
    vec3 source = 1.15 * texture(CRT_PASS, pp).rgb;
    vec3 bloom  = texture(Source, pp).rgb;
    source     += params.BLOOM_STRENGTH * bloom;
#endif
    FragColor = vec4(pow(clamp(source, 0.0, 1.0), vec3(1.0 / params.OUTPUT_GAMMA)), 1.0);
     /* TODO/FIXME - hacky clamp fix */
    if ( pp.x > 0.0001 && pp.x < 0.9999 && pp.y > 0.0001 && pp.y < 0.9999)
        FragColor.rgb = FragColor.rgb;
    else
FragColor.rgb = vec3(0.0);
FragColor.rgb *= (params.CURVATURE > 0.5) ? corner(pp) : 1.0;
if (params.shadowMask > 0.0)
        FragColor.rgb = pow(pow(FragColor.rgb, vec3(2.2)) * Mask(vTexCoord.xy * global.OutputSize.xy * 1.000001), vec3(1.0 / 2.2));
}
