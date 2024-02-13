#version 450

// based on Cole Cecil's
// Scaling Pixel ARt Without Destroying It
// https://colececil.io/blog/2017/scaling-pixel-art-without-destroying-it/

// This shader needs bilinear filtering

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
layout(location = 1) out vec2 textureCoords;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
   textureCoords = vTexCoord * params.SourceSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 textureCoords;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   // For the current screen pixel, find its location in relation to the nearest texels).
   vec2 locationWithinTexel = fract(textureCoords);
   
   // How many texels are in each pixel
   vec2 texelsPerPixel = params.OutputSize.zw * params.SourceSize.xy;
   
   // Determine how much (if at all) we need to interpolate between the colors of the nearest texels
   // No interpolation means it’s using nearest neighbor filtering.
   // Interpolation means it’s using bilinear filtering
   vec2 interpolationAmount = clamp(locationWithinTexel / texelsPerPixel, 0., 0.5)
         + clamp((locationWithinTexel - 1.) / texelsPerPixel + 0.5, 0., 0.5);
         
   // Given the interpolation amount, calculate and return the color of the pixel.
   vec2 finalTextureCoords = (floor(textureCoords) + interpolationAmount) * params.SourceSize.zw;
   FragColor = vec4(texture(Source, finalTextureCoords).rgb, 1.0);
}