#version 450

// A version of the LUT shader that loads 2 LUTs.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float LUT_selector_param;
} params;

#pragma parameter LUT_selector_param "LUT Selector" 1.0 1.0 2.0 1.0
int LUT_selector = int(params.LUT_selector_param);

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
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D SamplerLUT1;
layout(set = 0, binding = 4) uniform sampler2D SamplerLUT2;

// This shouldn't be necessary but it seems some undefined values can
// creep in and each GPU vendor handles that differently. This keeps
// all values within a safe range
vec4 mixfix(vec4 a, vec4 b, float c)
{
	return (a.z < 1.0) ? mix(a, b, c) : a;
}

void main()
{
   float LUT_Size = mix(textureSize(SamplerLUT1, 0).y, textureSize(SamplerLUT2, 0).y, params.LUT_selector_param - 1.0);
	vec4 imgColor = texture(Source, vTexCoord.xy);
	vec4 color1, color2 = vec4(0.,0.,0.,0.);
	float red, green, blue1, blue2, mixer = 0.0;
	
	red = ( imgColor.r * (LUT_Size - 1.0) + 0.4999 ) / (LUT_Size * LUT_Size);
	green = ( imgColor.g * (LUT_Size - 1.0) + 0.4999 ) / LUT_Size;
	blue1 = (floor( imgColor.b  * (LUT_Size - 1.0) ) / LUT_Size) + red;
	blue2 = (ceil( imgColor.b  * (LUT_Size - 1.0) ) / LUT_Size) + red;
	mixer = clamp(max((imgColor.b - blue1) / (blue2 - blue1), 0.0), 0.0, 32.0);
	
	if(LUT_selector == 1)
	{
   	color1 = texture( SamplerLUT1, vec2( blue1, green ));
   	color2 = texture( SamplerLUT1, vec2( blue2, green )); 
	}
	else
	{
		color1 = texture( SamplerLUT2, vec2( blue1, green ));
		color2 = texture( SamplerLUT2, vec2( blue2, green ));
	}
	FragColor = mixfix(color1, color2, mixer);
}
