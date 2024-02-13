#version 450

// vt220
// by sprash3
// https://www.shadertoy.com/view/XdtfzX

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#pragma parameter curvature "Curve Radius" 3.0 0.0 10.0 0.1
#pragma parameter width "Width" 1.0 0.0 2.0 0.01
#pragma parameter height "Height" 1.12 0.0 2.0 0.01
#pragma parameter smoothness "Border Blur" 1.0 0.0 10.0 0.1
#pragma parameter shine "Screen Reflection" 1.0 0.0 10.0 0.1
#pragma parameter blur_size "Reflection Blur" 3.0 0.0 5.0 0.05
#pragma parameter dimmer "Ambient Brightness" 0.5 0.0 1.0 0.05
#pragma parameter csize "Corner size" 0.045 0.0 0.07 0.01
#pragma parameter mask "Mask Type" 2.0 0.0 19.0 1.0
#pragma parameter mask_strength "Mask Strength" 0.5 0.0 1.0 0.05
#pragma parameter zoom "Viewing Distance" 0.85 0.0 2.0 0.01
#pragma parameter SCANLINE_SINE_COMP_B "Scanline Darkness" 0.15 0.0 1.0 0.05
#pragma parameter ntsc_toggle "NTSC Toggle" 0.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
   float ntsc_toggle, curvature, width, height, smoothness, shine, blur_size, dimmer, csize, mask, zoom, mask_strength, SCANLINE_BASE_BRIGHTNESS, SCANLINE_SINE_COMP_A, SCANLINE_SINE_COMP_B;
} global;

#define SCANLINE_SINE_COMP_B global.SCANLINE_SINE_COMP_B

int mask_picker = int(global.mask);
vec2 omega = vec2(3.141592654 * params.OutputSize.x, 2.0 * 3.141592654 * params.SourceSize.y);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
	vTexCoord = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D vt220_refpass;

#define iTime (float(params.FrameCount) / 60.0)
const vec3 iMouse = vec3(0.0);
#define iResolution params.OutputSize.xy

//#define LIGHTS_ON true
//#define LIGHTS_ON false
//#define LIGHTS_ON sin(fract(iTime/23.)+2.74) + 0.1*abs(sin(iTime*1000.)) <.0

#define WIDTH 0.48 * global.width
#define HEIGHT 0.3 * global.height
#define CURVE global.curvature
#define SMOOTH 0.004 * global.smoothness
#define SHINE 0.66 * global.shine

#define PHOSPHOR_COL vec4(0.75, 0.75, 0.75, 0.0)
#define BEZEL_COL vec4(0.8, 0.8, 0.6, 0.0)

#define REFLECTION_BLUR_ITERATIONS 5
#define REFLECTION_BLUR_SIZE 0.04 * global.blur_size

#include "../../../include/subpixel_masks.h"

vec3 scanline(vec3 res, vec2 coord)
{
   vec2 sine_comp = vec2(0.0, SCANLINE_SINE_COMP_B);
   vec3 scanline = res * ((1.0 - SCANLINE_SINE_COMP_B/3.) + dot(sine_comp * sin(coord * omega), vec2(1.0, 1.0)));
   return vec3(scanline.x, scanline.y, scanline.z);
}


float roundSquare(vec2 p, vec2 b, float r)
{
    return length(max(abs(p)-b,0.0))-r;
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 CurvedSurface(vec2 uv, float r)
{
    return r * uv/sqrt(r * r - dot(uv, uv));
}

float corner(vec2 coord)
{
	coord = (coord - vec2(0.5)) * 1.0 + vec2(0.5);
	coord = min(coord, vec2(1.0)-coord) * vec2(1.0, params.OutputSize.y/params.OutputSize.x);
	vec2 cdist = vec2(max(global.csize, max((1.0-smoothstep(100.0,600.0,800.0))*0.01,0.002)));
	coord = (cdist - min(coord,cdist));
	float dist = sqrt(dot(coord,coord));
	return clamp((cdist.x-dist)*800.0,0.0, 1.0);
}

// Coordinates for content
vec2 crtCurvA(vec2 uv) 
{
    float r = CURVE;
    if (iMouse.z > 0.) r *= exp(0.5 - iMouse.y/iResolution.y);
    uv = (uv / iResolution.xy - 0.5) / vec2(iResolution.y/iResolution.x, 1.) * 1.88 * global.zoom;
	uv = CurvedSurface(uv, r);
	uv *= 0.5 / vec2(WIDTH, HEIGHT);
   uv *= vec2(0.985,0.95);
    uv = (uv / 2.0) + 0.5;        
   	if (iMouse.z > 0.) uv.x -= iMouse.x/iResolution.x - 0.5;
    
	return uv;
}

// Coordinates for the rest
vec2 crtCurvB(vec2 uv, float r) 
{
    r = CURVE * r;
    if (iMouse.z > 0.) r *= exp(0.5-iMouse.y/iResolution.y);
    uv = (uv / iResolution.xy - 0.5) / vec2(iResolution.y/iResolution.x, 1.) * 2.0 * global.zoom;
	uv = CurvedSurface(uv, r);
    uv = (uv / 2.0) + 0.5;
  	if (iMouse.z > 0.) uv.x -= iMouse.x/iResolution.x - 0.5;
    
	return uv;
}

// Coordinates for the shine
vec2 crtCurvS(vec2 uv, float r) 
{
    r = CURVE * r;
    if (iMouse.z > 0.) r *= exp(0.5-iMouse.y/iResolution.y);
    uv = (uv / iResolution.xy - 0.5) / vec2(iResolution.y/iResolution.x, 1.) * 2.0 * global.zoom;
	uv = CurvedSurface(uv, r);
    uv = (uv / 2.0) + 0.5;
    
	return uv;
}


// standard roundSquare
float stdRS(vec2 uv, float r)
{
    return roundSquare(uv - 0.5, vec2(WIDTH, HEIGHT) + r, 0.05);
}

// Calculate normal to distance function and move along
// normal with distance to get point of reflection
vec2 borderReflect(vec2 p, float r)
{
    float eps = 0.0001;
    vec2 epsx = vec2(eps,0.0);
    vec2 epsy = vec2(0.0,eps);
    vec2 b = (1.+vec2(r,r))* 0.5;
    r /= 3.0;
    
    p -= 0.5;
    vec2 normal = vec2(roundSquare(p-epsx,b,r)-roundSquare(p+epsx,b,r),
                       roundSquare(p-epsy,b,r)-roundSquare(p+epsy,b,r))/eps;
    float d = roundSquare(p, b, r);
    p += 0.5;
    p = vec2(1.-p.x,p.y);
    return p + d*normal;
}

vec2 getCurves(in vec2 coord, out vec2 uvC, out vec2 uvS, out vec2 uvE)
{
    uvC = crtCurvA(coord);			// Content Layer
    uvS = crtCurvB(coord, 1.);		// Screen Layer
    uvE = crtCurvB(coord, 1.25);	// Enclosure Layer
    
    return uvC;
}

vec4 bezelGen(sampler2D iChannel0, vec2 fragCoord, vec4 image)
{
    vec4 c = vec4(0.0, 0.0, 0.0, 0.0);
    
// curvature needs to be applied to host shader
    vec2 uvC, uvS, uvE;  
    uvC = getCurves(fragCoord, uvC, uvS, uvE);
// end curvature

    vec4 c1 = vec4(0.0); vec4 c2 = vec4(0.0);
//    if (LIGHTS_ON) {
        // From my shader https://www.shadertoy.com/view/MtBXW3
        
   float ambient = 0.1;

        // Glass Shine 
        vec2 uvSh = crtCurvS(fragCoord, 1.);
    	c1 += max(0.0, SHINE - distance(uvSh, vec2(0.5, 1.0))) *
            smoothstep(SMOOTH/2.0, -SMOOTH/2.0, stdRS(uvS + vec2(0., 0.03), 0.0));

	    // Ambient
	    c1 += max(0.0, ambient - 0.5*distance(uvS, vec2(0.5,0.5))) *
	        smoothstep(SMOOTH, -SMOOTH, stdRS(uvS, 0.0));

	    // Enclosure Layer 
        uvSh = crtCurvS(fragCoord, 1.25);
    	vec4 b = vec4(0., 0., 0., 0.);
    	for(int i=0; i<12; i++)
			b += (clamp(BEZEL_COL + rand(uvSh+float(i))*0.05-0.025, 0., 1.) +
				rand(uvE+1.0+float(i))*0.25 * cos((uvSh.x-0.5)*3.1415*1.5))/12.;
        
        // Inner Border
        const float HHW = 0.5 * HEIGHT/WIDTH;
        
    	c1 += b/3.*( 1. + smoothstep(HHW - 0.025, HHW + 0.025, abs(atan(uvS.x-0.5, uvS.y-0.5))/3.1415) 
       		+ smoothstep(HHW + 0.025, HHW - 0.025, abs(atan(uvS.x-0.5, 0.5-uvS.y))/3.1415) )* 
			smoothstep(-SMOOTH, SMOOTH, stdRS(uvS, 0.0)) * 
			smoothstep(SMOOTH, -SMOOTH, stdRS(uvE, 0.05));
    
		// Inner Border Shine
  		c1 += (b - 0.4)* 
			smoothstep(-SMOOTH*2.0, SMOOTH*2.0, roundSquare(uvE-vec2(0.5, 0.505), vec2(WIDTH, HEIGHT) + 0.05, 0.05)) * 
			smoothstep(SMOOTH*2.0, -SMOOTH*2.0, roundSquare(uvE-vec2(0.5, 0.495), vec2(WIDTH, HEIGHT) + 0.05, 0.05));
        
    	// Outer Border
    	c1 += b * 
			smoothstep(-SMOOTH, SMOOTH, roundSquare(uvE-vec2(0.5, 0.5), vec2(WIDTH, HEIGHT) + 0.05, 0.05)) * 
			smoothstep(SMOOTH, -SMOOTH, roundSquare(uvE-vec2(0.5, 0.5), vec2(WIDTH, HEIGHT) + 0.15, 0.05)); 

    	// Outer Border Shine
		c1 += (b - 0.4)* 
			smoothstep(-SMOOTH*2.0, SMOOTH*2.0, roundSquare(uvE-vec2(0.5, 0.495), vec2(WIDTH, HEIGHT) + 0.15, 0.05)) * 
			smoothstep(SMOOTH*2.0, -SMOOTH*2.0, roundSquare(uvE-vec2(0.5, 0.505), vec2(WIDTH, HEIGHT) + 0.15, 0.05));
        
        // Table and room
        c1 += max(0. , (1. - 2.0* fragCoord.y/iResolution.y)) * vec4(1, 1, 1, 0.) *
            smoothstep(-0.25, 0.25, roundSquare(uvC - vec2(0.5, -0.2), vec2(WIDTH+0.25, HEIGHT-0.15), .1)) *
            smoothstep(-SMOOTH*2.0, SMOOTH*2.0, roundSquare(uvE-vec2(0.5, 0.5), vec2(WIDTH, HEIGHT) + 0.15, 0.05));
        
//    } else {
        // From my shader https://www.shadertoy.com/view/lt2SDK
        
   ambient = 0.2;

        // Ambient
	    c2 += max(0.0, ambient - 0.3*distance(uvS, vec2(0.5,0.5))) *
	        smoothstep(SMOOTH, -SMOOTH, stdRS(uvS, 0.0));
        
	    // Inner Border               
	  	c2 += BEZEL_COL * ambient * 0.7 *
	        smoothstep(-SMOOTH, SMOOTH, stdRS(uvS, 0.0)) * 
	        smoothstep(SMOOTH, -SMOOTH, stdRS(uvE, 0.05));
    
	    // Corner
	  	c2 -= (BEZEL_COL )* 
	        smoothstep(-SMOOTH*2.0, SMOOTH*10.0, stdRS(uvE, 0.05)) * 
	        smoothstep(SMOOTH*2.0, -SMOOTH*2.0, stdRS(uvE, 0.05));

	    // Outer Border
	    c2 += BEZEL_COL * ambient *
	       	smoothstep(-SMOOTH, SMOOTH, stdRS(uvE, 0.05)) * 
	        smoothstep(SMOOTH, -SMOOTH, stdRS(uvE, 0.15)); 
    
	    // Inner Border Reflection
	    for(int i = 0; i < REFLECTION_BLUR_ITERATIONS; i++)
	    {
	    	vec2 uvR = borderReflect(uvC + (vec2(rand(uvC+float(i)), rand(uvC+float(i)+0.1))-0.5)*REFLECTION_BLUR_SIZE, 0.05) ;
	    	c2 += (PHOSPHOR_COL - BEZEL_COL*ambient) * texture(iChannel0, 1.-vec2(uvR.x, uvR.y)) / float(REFLECTION_BLUR_ITERATIONS) * 
		        smoothstep(-SMOOTH, SMOOTH, stdRS(uvS, 0.0)) * 
				smoothstep(SMOOTH, -SMOOTH, stdRS(uvE, 0.05));
	    }
// commenting because mipmaps are a crapshoot with slang; TODO: try again with GLSL
//	    // Bloom using composed MipMaps
//	    c2 += textureLod(iChannel0, vec2(uvC.x, 1.0-uvC.y), 3.) * smoothstep(0., -SMOOTH*20., stdRS(uvS, -0.02)) * 0.5;
//	   	c2 += textureLod(iChannel0, vec2(uvC.x, 1.0-uvC.y), 4.) * smoothstep(0., -SMOOTH*20., stdRS(uvS, -0.02)) * 0.5;
//	    c2 += textureLod(iChannel0, vec2(uvC.x, 1.0-uvC.y), 5.) * smoothstep(0., -SMOOTH*20., stdRS(uvS, -0.02)) * 0.5;
//    }
    // mix light and dark for variable brightness
    c = mix(c2, c1, global.dimmer);

   if (uvC.x > 0. && uvC.x < 1. && uvC.y > 0. && uvC.y < 1.)
      c += vec4(image);
      return c;
}

void main()
{
   vec2 fragCoord = vec2(vTexCoord.x, 1.0-vTexCoord.y) * params.OutputSize.xy;
   vec2 uvC, uvS, uvE;  
   uvC = getCurves(fragCoord, uvC, uvS, uvE);
   
   vec3 sample_image = (global.ntsc_toggle > 0.5) ? texture(Source, vec2(uvC.x, 1.0-uvC.y)).rgb : texture(vt220_refpass, vec2(uvC.x, 1.0-uvC.y)).rgb;

   vec4 image = vec4(sample_image * corner(uvC) * mask_weights(gl_FragCoord.xy, global.mask_strength, mask_picker), 1.0);
   image.rgb = pow(image.rgb, vec3(2.5/2.2)); //CRT-like gamma correction
   image.rgb = scanline(image.rgb, uvC.xy); //apply scanlines
   FragColor = (global.ntsc_toggle > 0.5) ? bezelGen(Source, fragCoord.xy, image) : bezelGen(vt220_refpass, fragCoord.xy, image); //apply bezel
}
