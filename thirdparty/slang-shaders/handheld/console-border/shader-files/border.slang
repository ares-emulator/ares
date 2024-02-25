#version 450

// Created by inigo quilez - iq/2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// modified for slang border shader by hunterk

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	uint FrameCount;
	float box_scale;
	float location_x;
	float location_y;
	float in_res_x;
	float in_res_y;
	float border_on_top;
	float border_zoom_x;
	float border_zoom_y;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter box_scale "Image Scale" 4.0 1.0 10.0 1.0
#pragma parameter location_x "Viewport X Pos." 0.5 0.0 1.0 0.05
#pragma parameter location_y "Viewport Y Pos." 0.5 0.0 1.0 0.05
#pragma parameter in_res_x "Viewport Size X" 320.0 100.0 600.0 1.0
#pragma parameter in_res_y "Viewport Size Y" 240.0 64.0 512.0 1.0
#pragma parameter border_on_top "Show Viewport" 1.0 0.0 1.0 1.0
#pragma parameter border_zoom_x "Border Zoom X" 1.0 0.0 4.0 0.01
#pragma parameter border_zoom_y "Border Zoom Y" 1.0 0.0 4.0 0.01

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 screen_coord;

void main()
{
   gl_Position = global.MVP * Position;
   vec2 corrected_size = vec2(params.in_res_x, params.in_res_y);
   vec2 scale = (params.OutputSize.xy / corrected_size) / params.box_scale;
   vec2 middle = vec2(params.location_x, params.location_y);
   vec2 diff = TexCoord.xy - middle;
   screen_coord = middle + diff * scale;
   middle = vec2(0.4999, 0.4999);
   vTexCoord = middle + diff * vec2(params.border_zoom_x, params.border_zoom_y);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 screen_coord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D Original;
layout(set = 0, binding = 4) uniform sampler2D BORDER;

void main()
{
vec4 screen = texture(Source, screen_coord); //the main video screen
vec4 background = vec4(texture(BORDER, vTexCoord)); //put your background function's output here
if ( screen_coord.x < 0.9999 && screen_coord.x > 0.0001 && screen_coord.y < 0.9999 && screen_coord.y > 0.0001 && params.border_on_top > 0.5 )
background.a *= 0.0;
   FragColor = vec4(mix(screen, background, background.a));
}