#version 450

/*

- crt-blurPi slang shader -

Looks good on low res screens (640 x 480 or less), providing screen space scanlines.
Developed on and for the Raspberry Pi.

Made by Oriol Ferrer Mesià (armadillu)
http://uri.cat

MIT license
*/


layout(push_constant) uniform Push{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	//params....
	float scanlineGain;
	float rgbExtraGain;
	float blurGain;	
	float blurRadius;
}params;


#pragma name crt-blurPi
#pragma parameter scanlineGain "scanlineGain" 	0.30 	0.0 	1.0 	0.05
#pragma parameter rgbExtraGain "rgbExtraGain" 	0.10 	0.0 	1.0 	0.05
#pragma parameter blurGain "blurGain" 			0.15 	0.0 	1.0 	0.05
#pragma parameter blurRadius "blurRadius" 		1.5 	0.1 	3.0 	0.1

layout(std140, set = 0, binding = 0) uniform UBO{
	mat4 MVP;
}global;



/////////////////////////////////////////////////////////////////////////////////////////
#pragma stage vertex
/////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 dot_size;

void main(){
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
	dot_size = params.SourceSize.zw;
}


/////////////////////////////////////////////////////////////////////////////////////////
#pragma stage fragment
/////////////////////////////////////////////////////////////////////////////////////////

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 dot_size;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main(){


	const vec2 off = params.blurRadius * dot_size;
	float srcGain = 1.0f - 0.75f * params.blurGain;

	vec4 blurred = srcGain * (1.0f + params.rgbExtraGain) * texture(Source, vTexCoord) + 
   				0.25f * params.blurGain * texture(Source, vTexCoord + vec2(-off.x ,0.0f)) +
   				0.25f * params.blurGain * texture(Source, vTexCoord + vec2(off.x, 0.0f)) +
   				0.25f * params.blurGain * texture(Source, vTexCoord + vec2(0.0f, off.y))
   				;

	float scanLine = mod(int(vTexCoord.y * params.OutputSize.y), 2.0f);

	FragColor = mix(1.0f, scanLine * scanLine, params.scanlineGain) * blurred;

}