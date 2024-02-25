#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

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
layout(set = 0, binding = 3) uniform sampler2D COLOR_PALETTE;

void main()
{
     //sample the input textures

     vec4 out_color = texture(Source, vTexCoord.xy);

     //input grayscale values:
     //0.000 black - 0.333 medium gray - 0.667 light gray - 1.000 white
     //multiply grayscale value by 3 to obtain int value in range [0, 3], acts as index for arrays storing custom palette UV coordinates and alpha value

     vec2 palette_coordinate = vec2(0.5, (abs(1 - out_color.r) * 0.75) + 0.125);	//directly map input grayscale value to color palette y coordinate
     out_color = vec4(texture(COLOR_PALETTE, palette_coordinate).rgb, ceil(abs(1.0 - out_color.r)));	//sample color from palette, alpha = 1 if color is not transparent

     FragColor = out_color;
}
