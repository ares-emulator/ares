#version 450

// NTSC composite simulator for BlastEm, now with comb filter
// Shader by Sik, based on BlastEm's default shader
//
// Now with gamma correction (NTSC = 2.5 gamma, sRGB = 2.2 gamma*)
// *sorta, sRGB isn't exactly a gamma curve, but close enough
//
// It works by converting from RGB to YIQ and then encoding it into NTSC, then
// trying to decode it back. The lossy nature of the encoding process results in
// the rainbow effect. It also accounts for the differences between H40 and H32
// mode as it computes the exact colorburst cycle length.
//
// This shader tries to work around the inability to keep track of previous
// pixels by sampling seven points (in 0.25 colorburst cycle intervals), that
// seems to be enough to give decent filtering (four samples are used for
// low-pass filtering, but we need seven because decoding chroma also requires
// four samples so we're filtering over overlapping samples... just see the
// comments in the I/Q code to understand).
//
// The comb filter works by comparing against the previous scanline (which means
// sampling twice). This is done at the composite signal step, i.e. before the
// filtering to decode back YIQ is performed.
//
// Thanks to Tulio Adriano for helping compare against real hardware on a CRT.
//******************************************************************************

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float comb_str, gamma_corr, scan_toggle;
} params;

#pragma parameter comb_str "Combing Filter Strength" 0.8 0.0 1.0 0.01

// How strong is the comb filter when reducing crosstalk between Y and IQ.
// 0% (0.0) means no separation at all, 100% (1.0) means complete filtering.
// 80% seems to approximate a model 1, while 90% is closer to a model 2 or 32X.
float comb_strength = params.comb_str;

#pragma parameter gamma_corr "Gamma Correction (2.2 = passthru)" 2.2 1.0 4.0 0.05
// Gamma of the TV to simulate.
float gamma_correction = params.gamma_corr;

#pragma parameter scan_toggle "Scanline Toggle" 0.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 texcoord;
layout(location = 1) out vec2 texsize;
layout(location = 2) out int curfield;
layout(location = 3) out int scanlines;
layout(location = 4) out float width;
layout(location = 5) out float interlaced;

void main()
{
   gl_Position = global.MVP * Position;
   texcoord = TexCoord;
   texsize = params.SourceSize.xy;
   width = params.SourceSize.x;
   curfield = int(mod(params.FrameCount, 2));
   scanlines = int(params.scan_toggle);
   interlaced = (params.OriginalSize.y > 400.0) ? 1.0 : 0.0;
}

#pragma stage fragment
layout(location = 0) in vec2 texcoord;
layout(location = 1) in vec2 texsize;
layout(location = 2) flat in int curfield;
layout(location = 3) flat in int scanlines;
layout(location = 4) in float width;
layout(location = 5) in float interlaced;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// Converts from RGB to YIQ

vec3 rgba2yiq(vec4 rgba)
{
	return vec3(
		rgba[0] * 0.3 + rgba[1] * 0.59 + rgba[2] * 0.11,
		rgba[0] * 0.599 + rgba[1] * -0.2773 + rgba[2] * -0.3217,
		rgba[0] * 0.213 + rgba[1] * -0.5251 + rgba[2] * 0.3121
	);
}

// Encodes YIQ into composite

float yiq2raw(vec3 yiq, float phase)
{
	return yiq[0] + yiq[1] * sin(phase) + yiq[2] * cos(phase);
}


// Converts from YIQ to RGB

vec4 yiq2rgba(vec3 yiq)
{
	return vec4(
		yiq[0] + yiq[1] * 0.9469 + yiq[2] * 0.6236,
		yiq[0] - yiq[1] * 0.2748 - yiq[2] * 0.6357,
		yiq[0] - yiq[1] * 1.1 + yiq[2] * 1.7,
		1.0
	);
}


void main()
{
	// The coordinate of the pixel we're supposed to access
	// In interlaced mode, the entire screen is shifted down by half a scanline,
	// y_offset is used to account for this.
	float y_offset = interlaced * float(curfield) * -0.5 / texsize.y;
	float x = texcoord.x;
	float y = texcoord.y + y_offset;
	
	// Horizontal distance of half a colorburst cycle
	float factorX = (1.0 / texsize.x) / 170.667 * 0.5 * width;
	
	// sRGB approximates a gamma ramp of 2.2 while NTSC has a gamma of 2.5
	// Use this value to do gamma correction of every RGB value
	float gamma = gamma_correction / 2.2;
	
	// Where we store the sampled pixels.
	// [0] = current pixel
	// [1] = 1/4 colorburst cycles earlier
	// [2] = 2/4 colorburst cycles earlier
	// [3] = 3/4 colorburst cycles earlier
	// [4] = 1 colorburst cycle earlier
	// [5] = 1 1/4 colorburst cycles earlier
	// [6] = 1 2/4 colorburst cycles earlier
	float phase[7];		// Colorburst phase (in radians)
	float raw_y[7];    // Luma isolated from raw composite signal
	float raw_iq[7];   // Chroma isolated from raw composite signal
	
	// Sample all the pixels we're going to use
	for (int n = 0; n < 7; n++, x -= factorX * 0.5) {
		// Compute colorburst phase at this point
		phase[n] = x / factorX * 3.1415926;
		
		// Y coordinate one scanline above
		// Apparently GLSL doesn't allow a vec2 with a full expression for
		// texture samplers? Whatever, putting it here (also makes the code
		// below a bit easier to read I guess?)
		float y_above = y - texcoord.y / texsize.y;
		
		// Get the raw composite data for this scanline
		float raw1 = yiq2raw(rgba2yiq(
		                     texture(Source, vec2(x, y))
		                     ), phase[n]);
		
		// Get the raw composite data for scanline above
		// Note that the colorburst phase is shifted 180°
		float raw2 = yiq2raw(rgba2yiq(
		                     texture(Source, vec2(x, y_above))
		                     ), phase[n] + 3.1415926);
		
		// Comb filter: isolate Y and IQ using the above two.
		// Luma is obtained by adding the two scanlines, chroma will cancel out
		// because chroma will be on opposite phases.
		// Chroma is then obtained by cancelling this scanline from the luma
		// to reduce the crosstalk. We don't cancel it entirely though since the
		// filtering isn't perfect (which is why the rainbow leaks a bit).
		raw_y[n] = (raw1 + raw2) * 0.5;
		raw_iq[n] = raw1 - (raw1 + raw2) * (comb_strength * 0.5);
	}
	
	// Decode Y by averaging over the last whole sampled cycle (effectively
	// filtering anything above the colorburst frequency)
	float y_mix = (raw_y[0] + raw_y[1] + raw_y[2] + raw_y[3]) * 0.25;
	
	// Decode I and Q (see page below to understand what's going on)
	// https://codeandlife.com/2012/10/09/composite-video-decoding-theory-and-practice/
	//
	// Retrieving I and Q out of the raw signal is done like this
	// (use sin for I and cos for Q):
	//
	//    0.5 * raw[0] * sin(phase[0]) + 0.5 * raw[1] * sin(phase[1]) +
	//    0.5 * raw[2] * sin(phase[2]) + 0.5 * raw[3] * sin(phase[3])
	//
	// i.e. multiply each of the sampled quarter cycles against the reference
	// wave and average them (actually double that because for some reason
	// that's needed to get the correct scale, hence 0.5 instead of 0.25)
	//
	// That turns out to be blocky tho, so we opt to filter down the chroma...
	// which requires doing the above *four* times if we do it the same way as
	// we did for luminance (note that 0.125 = 1/4 of 0.5):
	//
	//    0.125 * raw[0] * sin(phase[0]) + 0.125 * raw[1] * sin(phase[1]) +
	//    0.125 * raw[2] * sin(phase[2]) + 0.125 * raw[3] * sin(phase[3]) +
	//    0.125 * raw[1] * sin(phase[1]) + 0.125 * raw[2] * sin(phase[2]) +
	//    0.125 * raw[3] * sin(phase[3]) + 0.125 * raw[4] * sin(phase[4]) +
	//    0.125 * raw[2] * sin(phase[2]) + 0.125 * raw[3] * sin(phase[3]) +
	//    0.125 * raw[4] * sin(phase[4]) + 0.125 * raw[5] * sin(phase[5]) +
	//    0.125 * raw[3] * sin(phase[3]) + 0.125 * raw[4] * sin(phase[4]) +
	//    0.125 * raw[5] * sin(phase[5]) + 0.125 * raw[6] * sin(phase[6])
	//
	// There are a lot of repeated values there that could be merged into one,
	// what you see below is the resulting simplification.
	
	float i_mix =
		0.125 * raw_iq[0] * sin(phase[0]) +
		0.25  * raw_iq[1] * sin(phase[1]) +
		0.375 * raw_iq[2] * sin(phase[2]) +
		0.5   * raw_iq[3] * sin(phase[3]) +
		0.375 * raw_iq[4] * sin(phase[4]) +
		0.25  * raw_iq[5] * sin(phase[5]) +
		0.125 * raw_iq[6] * sin(phase[6]);
	
	float q_mix =
		0.125 * raw_iq[0] * cos(phase[0]) +
		0.25  * raw_iq[1] * cos(phase[1]) +
		0.375 * raw_iq[2] * cos(phase[2]) +
		0.5   * raw_iq[3] * cos(phase[3]) +
		0.375 * raw_iq[4] * cos(phase[4]) +
		0.25  * raw_iq[5] * cos(phase[5]) +
		0.125 * raw_iq[6] * cos(phase[6]);
	
	// Convert YIQ back to RGB and output it
	FragColor = pow(yiq2rgba(vec3(y_mix, i_mix, q_mix)),
	                   vec4(gamma, gamma, gamma, 1.0));
	
	// If you're curious to see what the raw composite signal looks like,
	// comment out the above and uncomment the line below instead
	//gl_FragColor = vec4(raw[0], raw[0], raw[0], 1.0);
	
	// Basic scanlines effect. This is done by multiplying the color against a
	// "half sine" wave so the center is brighter and a narrow outer area in
	// each scanline is noticeable darker.
	// The weird constant in the middle line is 1-sqrt(2)/4 and it's used to
	// make sure that the average multiplied value across the whole screen is 1
	// to preserve the original brightness.
	if (scanlines != 0) {
		float mult = sin(y * texsize.y * 3.1415926);
		mult = abs(mult) * 0.5 + 0.646446609;
		FragColor *= vec4(mult, mult, mult, 1.0);
	}	
}
