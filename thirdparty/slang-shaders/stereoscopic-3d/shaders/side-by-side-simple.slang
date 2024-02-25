#version 450

// simple shader to view content on side-by-side 3D displays
// by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float eye_sep, y_loc, BOTH, ana_zoom, WIDTH, HEIGHT, warpX, warpY, pulfrich;
} params;

#pragma parameter eye_sep "Eye Separation" 0.30 -1.0 5.0 0.05
#define eye_sep params.eye_sep
#pragma parameter y_loc "Vertical Placement" 0.25 -1.0 1.0 0.01
#define y_loc params.y_loc
#pragma parameter BOTH "Horizontal Placement" 0.51 -2.0 2.0 0.005
#define BOTH params.BOTH
#pragma parameter ana_zoom "Zoom" 0.75 -2.0 2.0 0.05
#define ana_zoom params.ana_zoom
#pragma parameter WIDTH "Side-by-Side Image Width" 3.05 1.0 7.0 0.05
#define WIDTH params.WIDTH
#pragma parameter HEIGHT "Side-by-Side Image Height" 2.0 1.0 5.0 0.1
#define HEIGHT params.HEIGHT
#pragma parameter warpX "Lens Warp Correction X" 0.1 0.0 0.5 0.05
#define warpX params.warpX
#pragma parameter warpY "Lens Warp Correction Y" 0.1 0.0 0.5 0.05
#define warpY params.warpY
#pragma parameter pulfrich "Pulfrich Effect" 0.0 0.0 0.5 0.25
#define pulfrich params.pulfrich

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
   vTexCoord = ((TexCoord.xy - 0.5) * ana_zoom + 0.5) * vec2(WIDTH, HEIGHT) - vec2(BOTH, 0.0);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//distortion
vec2 Warp(vec2 pos){
  pos.xy = pos.xy * 2.0-1.0;    
  pos.xy *= vec2(1.0+(pos.y*pos.y)*warpX,1.0+(pos.x*pos.x)*warpY);
  return pos*0.5+0.5;}

void main()
{
    vec2 warpCoord1 = Warp(vTexCoord.xy - vec2(eye_sep,  y_loc));
    vec2 warpCoord2 = Warp(vTexCoord.xy + vec2(eye_sep, -y_loc));
    vec4 frame1 = texture(Source, warpCoord1);
    vec4 frame2 = texture(Source, warpCoord2) * (1.0 - pulfrich);

    vec4 final = vec4(frame1 + frame2);
    FragColor = final;
}
