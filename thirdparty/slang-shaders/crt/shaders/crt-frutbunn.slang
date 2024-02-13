#version 450

/*
Adapted for RetroArch from frutbunn's "Another CRT shader" from shadertoy:
https://www.shadertoy.com/view/XdyGzR
*/ 

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float CURVATURE;
	float SCANLINES;
	float CURVED_SCANLINES;
	float LIGHT;
	float light;
	float blur;
} params;

#pragma parameter CURVATURE "Curvature Toggle" 1.0 0.0 1.0 1.0
#pragma parameter SCANLINES "Scanlines Toggle" 1.0 0.0 1.0 1.0
#pragma parameter CURVED_SCANLINES "Scanline Curve Toggle" 1.0 0.0 1.0 1.0
#pragma parameter LIGHT "Vignetting Toggle" 1.0 0.0 1.0 1.0
#pragma parameter light "Vignetting Strength" 9.0 0.0 20.0 1.0'
#pragma parameter blur "Blur Strength" 1.0 0.0 8.0 0.05

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
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//#define CURVATURE 1. //moved to parameter
//#define SCANLINES 1. //moved to parameter
//#define CURVED_SCANLINES 1. //moved to parameter
#define BLURRED 1.
//#define LIGHT 1. //moved to parameter

const float gamma = 1.;
const float contrast = 1.;
const float saturation = 1.;
const float brightness = 1.;

//const float light = 9.; //moved to parameter
//const float blur = 1.0; //moved to parameter

// Sigma 1. Size 3
vec3 gaussian(in vec2 uv, in sampler2D tex, in vec2 iResolution) {
    float b = params.blur / (iResolution.x / iResolution.y);

    vec3 col = texture(tex, vec2(uv.x - b/iResolution.x, uv.y - b/iResolution.y) ).rgb * 0.077847;
    col += texture(tex, vec2(uv.x - b/iResolution.x, uv.y) ).rgb * 0.123317;
    col += texture(tex, vec2(uv.x - b/iResolution.x, uv.y + b/iResolution.y) ).rgb * 0.077847;

    col += texture(tex, vec2(uv.x, uv.y - b/iResolution.y) ).rgb * 0.123317;
    col += texture(tex, vec2(uv.x, uv.y) ).rgb * 0.195346;
    col += texture(tex, vec2(uv.x, uv.y + b/iResolution.y) ).rgb * 0.123317;

    col += texture(tex, vec2(uv.x + b/iResolution.x, uv.y - b/iResolution.y) ).rgb * 0.077847;
    col += texture(tex, vec2(uv.x + b/iResolution.x, uv.y) ).rgb * 0.123317;
    col += texture(tex, vec2(uv.x + b/iResolution.x, uv.y + b/iResolution.y) ).rgb * 0.077847;

    return col;
}

void main()
{
    vec2 st = vTexCoord - vec2(0.5);
    
    // Curvature/light
    float d = length(st*.5 * st*.5);
	vec2 uv;
if (params.CURVATURE > 0.5){
    uv = st*d + st*.935;
}else{
    uv = st;
}
    
    // CRT color blur
#ifdef BLURRED
    vec3 color = gaussian(uv+.5, Source, params.SourceSize.xy * 2.0);
#else
    vec3 color = texture(Source, uv+.5).rgb;
#endif

    // Light
if (params.LIGHT > 0.5){
    float l = 1. - min(1., d*params.light);
    color *= l;
}

    // Scanlines
	float y;
if (params.CURVED_SCANLINES > 0.5){
    y = uv.y;
}else{
    y = st.y;
}

    float showScanlines = 1.;
//    if (params.SourceSize.y<360.) showScanlines = 0.;
    
if (params.SCANLINES > 0.5){
    float s = 2.5 + params.OutputSize.y * params.SourceSize.w;//1. - smoothstep(params.SourceSize.x, params.OutputSize.x, params.SourceSize.y) + 4.;//1. - smoothstep(320., 1440., params.SourceSize.y) + 4.;
    float j = cos(y*params.SourceSize.y*s)*.25; // values between .01 to .25 are ok.
    color = abs(showScanlines-1.)*color + showScanlines*(color - color*j);
//    color *= 1. - ( .01 + ceil(mod( (st.x+.5)*params.SourceSize.x, 3.) ) * (.995-1.01) )*showScanlines;
}

    // Border mask
if (params.CURVATURE > 0.5){
        float m = max(0.0, 1. - 2.*max(abs(uv.x), abs(uv.y) ) );
        m = min(m*200., 1.);
        color *= m;
}

	FragColor = vec4(color, 1.0);//vec4(max(vec3(.0), min(vec3(1.), color)), 1.);
}