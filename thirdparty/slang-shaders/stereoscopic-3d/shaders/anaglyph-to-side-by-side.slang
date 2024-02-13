#version 450

// Anaglyph to Side-by-Side
// by hunterk
// license: public domain
//
// This shader is designed to convert Mednafen-VB's anaglyph 3D output to
// side-by-side 3D for use with VR headsets, such as Oculus Rift and Google Cardboard, and 3D TVs

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float eye_sep;
	float y_loc;
	float BOTH;
	float ana_zoom;
	float WIDTH;
	float HEIGHT;
	float palette;
	float warp_enable;
	float warpX;
	float warpY;
	float anaglyph_selector;
	float eye_swap;
} params;

#pragma parameter eye_sep "Eye Separation" 0.5 -1.0 5.0 0.01
#pragma parameter y_loc "Vertical Placement" 0.50 -1.0 1.0 0.025
#pragma parameter BOTH "Horizontal Placement" 1.0 -2.0 2.0 0.005
#pragma parameter ana_zoom "Zoom" 0.75 -2.0 2.0 0.05
#pragma parameter WIDTH "Side-by-Side Image Width" 3.05 1.0 7.0 0.05
#pragma parameter HEIGHT "Side-by-Side Image Height" 2.0 1.0 5.0 0.1
#pragma parameter palette "Red Palette Toggle" 0.0 0.0 1.0 1.0
#pragma parameter warp_enable "Lens Correction" 1.0 0.0 1.0 1.0
#pragma parameter warpX "warpX" 0.3 0.0 0.5 0.05
#pragma parameter warpY "warpY" 0.3 0.0 0.5 0.05
#pragma parameter anaglyph_selector "Anaglyph Mode" 0.0 0.0 2.0 1.0
#pragma parameter eye_swap "Swap Eyes" 0.0 0.0 1.0 1.0

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
   vTexCoord = ((TexCoord.xy * 1.00001 - 0.5) * params.ana_zoom + 0.5) * vec2(params.WIDTH, params.HEIGHT) - vec2(params.BOTH, 0.0);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//warp function from Lottes' scanline shader
vec2 Warp(vec2 pos){
  pos=pos*2.0-1.0;    
  pos*=vec2(1.0+(pos.y*pos.y)*params.warpX,1.0+(pos.x*pos.x)*params.warpY);
  return pos*0.5+0.5;}

void main()
{
	vec2 coord1 = (params.warp_enable > 0.5) ? Warp((vTexCoord.xy - vec2(params.eye_sep, params.y_loc))) : (vTexCoord.xy - vec2(params.eye_sep, params.y_loc));
	vec2 coord2 = (params.warp_enable > 0.5) ? Warp((vTexCoord.xy + vec2(params.eye_sep, -params.y_loc))) : (vTexCoord.xy + vec2(params.eye_sep, -params.y_loc));

	vec4 frame1;
	vec4 frame2;
if (params.eye_swap < 0.5){
	frame1 = texture(Source, coord1);
	frame2 = texture(Source, coord2);}
else {
	frame1 = texture(Source, coord2);
	frame2 = texture(Source, coord1);}

if (params.anaglyph_selector == 0.0){ //red/blue and red/cyan
	frame1.rgb = vec3(frame1.r);
	frame2.rgb = vec3(max(frame2.g, frame2.b));}
else if (params.anaglyph_selector == 1.0){ //red/green and green/magenta
	frame1.rgb = vec3(max(frame1.r, frame1.b));
	frame2.rgb = vec3(frame2.g);}
else { //yellow and blue
	frame1.rgb = vec3(max(frame1.r, frame1.g));
	frame2.rgb = vec3(frame2.b);}

if (params.palette > 0.5)
	FragColor = frame1 + frame2;
else
	FragColor = vec4(frame1.r + frame2.r, 0.0, 0.0, 1.0);
}