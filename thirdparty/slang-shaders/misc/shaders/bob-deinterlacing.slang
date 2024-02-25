#version 450

/*
   Bob-Deinterlacing
   Author: hunterk
   License: Public domain
   
   Note: This shader is designed to work with the typical interlaced output from
   an emulator, which displays both even and odd fields twice.
   
   As such, it is inappropriate for general video use unless the video has
   already been similarly woven beforehand.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float scale, ghost;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// I wish there were some way to make this automatic...
#pragma parameter scale "Deinterlacing Scale" 1.0 1.0 16.0 1.0

#pragma parameter ghost "Blend Frames to Hide Bobbing" 0.0 0.0 1.0 1.0

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float is_interlaced;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   is_interlaced = float(params.OriginalSize.y > 400.0 * params.scale);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float is_interlaced;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;

void main()
{
   // snap to the center of the underlying texel
   vec2 uv = vTexCoord * params.SourceSize.xy - 0.5;
   uv = (floor(uv) + 0.5001) * params.SourceSize.zw;
   
   // go ahead and sample the texture and early return if not interlaced
	vec4 current = texture(Source, uv);
	FragColor = current;
	if(!bool(is_interlaced)) return;
   
   float scale = params.scale;

	// work our way down the vertical axis and skip up 1 pixel every other frame
	float y = (params.SourceSize.y / scale) * uv.y + mod(float(params.FrameCount), 2.0);
	
	// deinterlace the current frame
	vec4 offset = texture(Source, uv + vec2(0.0, params.SourceSize.w * scale));
	vec4 deint_current = (mod(y, 2.0) > 0.99999) ? offset : current;
	
	FragColor = deint_current;
	// early return if we don't want to do any frame-blending
	if(!bool(params.ghost)) return;
	
	// previous frame needs an opposite tick from the current frame
	float y2 = (params.SourceSize.y / scale) * uv.y + clamp(1.0 - mod(float(params.FrameCount), 2.0), 0.0, 1.0);
	
	// deinterlace previous frame
	vec4 prev = texture(OriginalHistory1, uv);
	vec4 prev_offset = texture(OriginalHistory1, uv + vec2(0.0, params.SourceSize.w * scale));
	vec4 deint_prev = (mod(y2, 2.0) > 0.99999) ? prev_offset : prev;
	
	FragColor = mix(deint_current, deint_prev, 0.5);
}
