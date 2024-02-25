#version 450

/*
   b-spline-y Shader

   Copyright (C) 2011-2022 Hyllian - sergiogdb@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float SUPERXBR_ON;
} params;

#pragma parameter SUPERXBR_ON 		"          Super-XBR ON" 1 0 1 1
#define SUPERXBR_ON params.SUPERXBR_ON

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// B-Spline bicubic parameters
const float  B =  0.0; 
const float  C =  1.0;

const mat4 INV  = mat4(            (-B - 6.0*C)/6.0,         (3.0*B + 12.0*C)/6.0,     (-3.0*B - 6.0*C)/6.0,             B/6.0,
                                        (12.0 - 9.0*B - 6.0*C)/6.0, (-18.0 + 12.0*B + 6.0*C)/6.0,                      0.0, (6.0 - 2.0*B)/6.0,
                                       -(12.0 - 9.0*B - 6.0*C)/6.0, (18.0 - 15.0*B - 12.0*C)/6.0,      (3.0*B + 6.0*C)/6.0,             B/6.0,
                                                   (B + 6.0*C)/6.0,                           -C,                      0.0,               0.0);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;

void main()
{
   gl_Position = global.MVP * Position;
   
   vec2 ps = vec2(params.SourceSize.z, params.SourceSize.w);
	float dx = ps.x;
	float dy = ps.y;

        vTexCoord = TexCoord * 1.0001 - vec2(0.0, 0.5)*ps;

	t1 = vTexCoord.xyxy + vec4( 0.0, -dy, 0.0,    0.0); 
	t2 = vTexCoord.xyxy + vec4( 0.0,  dy, 0.0, 2.0*dy);

}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D XbrSource;

void main()
{
// 	// HSM Added
// 	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
// 	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

// 	vec2 cache_bounds_coord = SCREEN_COORD;

// // If it's the potato preset render the whole frame
// #ifndef IS_POTATO_PRESET
// #ifndef IS_NO_REFLECT_PRESET
// 	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
// 	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
// 						TUBE_DIFFUSE_SCALE, 
// 						TUBE_SCALE, 
// 						SCREEN_ASPECT, 
// 						false,
// 						BEZEL_OUTSIDE_SCALE,
// 						BEZEL_OUTSIDE_COORD, 
// 						BEZEL_OUTSIDE_CURVED_COORD, 
// 						FRAME_OUTSIDE_CURVED_COORD);
// 	cache_bounds_coord = (FRAME_OUTSIDE_CURVED_COORD - 0.5) * 0.9 + 0.5;
// #endif
// #endif

// 	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
// 	{
//         FragColor = vec4(0);
//         return;
// 	}

   if (SUPERXBR_ON < 0.5)
	{
		FragColor = texture(XbrSource, vTexCoord);
		return;
	}

  vec2 fp = fract(vTexCoord*params.SourceSize.xy);

  vec3 C0 = texture(Source, t1.xy).xyz;
  vec3 C1 = texture(Source, t1.zw).xyz;
  vec3 C2 = texture(Source, t2.xy).xyz;
  vec3 C3 = texture(Source, t2.zw).xyz;

  vec4 Py    = vec4(fp.y*fp.y*fp.y, fp.y*fp.y, fp.y, 1.0) * INV;
  vec3 color = mat4x3(C0, C1, C2, C3) * Py;

  FragColor = vec4(color, 1.0);
}
