#version 450

// Insert a configurable count (1, 2, 3...) of interpolated display (high-res)
// pixel rows between each pair of adjacent source (low-res) pixels.
// by decavoid


#pragma parameter PixelCount "Pixel Count" 2.0 1.0 8.0 1.0

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float PixelCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 PixelCoords;

void main()
{
   gl_Position = global.MVP * Position;
	PixelCoords = TexCoord * params.SourceSize.xy; // NES x: [0; 256], y: [0; 240]
}

#pragma stage fragment
layout(location = 0) in vec2 PixelCoords;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// uncomment to see a red grid of modified pixels
//#define DEBUG_DRAW_EDGES

vec4 sizeScale = params.OutputSize / params.OriginalSize;
vec2 interpolationRangeHalf = params.PixelCount * 0.5 * sizeScale.zw;
float stepPerRow = 1.0 / (params.PixelCount + 1.0);

void main()
{
	vec2 coordAtPixelCenter = (floor(PixelCoords) + vec2(0.5));
	vec2 coordBetweenPixels = round(PixelCoords);
	vec2 origOffset = PixelCoords - coordBetweenPixels + 1e-3; // [-0.5; 0.5]

	vec2 needInterpolate = step(abs(origOffset), interpolationRangeHalf);

	// if needInterpolate == 0, disable interpolation by choosing coordAtPixelCenter.
	//
	// if needInterpolate == 1, transform origOffset.x
	// from range [-interpolationRangeHalf.x; interpolationRangeHalf.x]
	// to range (-0.5; 0.5)
	vec2 segmentIndex = floor((origOffset + interpolationRangeHalf) * sizeScale.xy);
	vec2 transformedOffset = stepPerRow * (segmentIndex + 1) - 0.5;
	vec2 interpolatedCoord = coordBetweenPixels + transformedOffset;

	vec2 newCoord = (1 - needInterpolate) * coordAtPixelCenter + needInterpolate * interpolatedCoord;
	vec2 newTexCoord = newCoord * params.SourceSize.zw;

	FragColor = texture(Source, newTexCoord);

#ifdef DEBUG_DRAW_EDGES
	if (needInterpolate.x + needInterpolate.y > 0)
		FragColor.r = 1;
#endif
}
