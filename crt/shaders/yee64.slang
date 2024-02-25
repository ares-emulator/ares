#version 450

// ported from ReShade

layout(push_constant) uniform Push
{
	float viewSizeHD;
	float brightness;
	float intensityR;
	float intensityG;
	float intensityB;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#pragma parameter viewSizeHD "Min Dimming Res" 720.0 0.0 2190.0 1.0
int viewSizeHD = int(params.viewSizeHD);
#pragma parameter brightness "CRT Brightness" 1.5 0.0 5.0 0.1
#pragma parameter intensityR "Red Dimming Intensity" 1.2 0.0 2.0 0.1
#pragma parameter intensityG "Green Dimming Intensity" 0.9 0.0 2.0 0.1
#pragma parameter intensityB "Blue Dimming Intensity" 0.9 0.0 2.0 0.1

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 FinalViewportSize;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{

	vec2 textureSize = params.SourceSize.xy; // Size of Internal Framebuffer
	vec2 viewSize = global.FinalViewportSize.xy; // Size of final Viewport
	vec2 pixelSize = params.OriginalSize.xy; // Size of Game resolution;

    vec2 texelPos = (textureSize.xy / pixelSize.xy) * vTexCoord.xy;
    vec4 size     = (pixelSize.xy / textureSize.xy).xyxy * texelPos.xyxy;
    vec2 exp    = size.zw * textureSize.xy + -floor(size.zw * textureSize.xy) + -0.5;

	vec4 factor  =vec4(0.0,0.0,0.0,0.0);
	factor.x = pow(2.0, pow(-1.0 - exp.x, 2.0) * (-3.0));
    factor.y = pow(2.0, pow( 1.0 - exp.x, 2.0) * (-3.0));
    factor.z = pow(2.0, pow(-2.0 - exp.x, 2.0) * (-3.0));
    factor.w = pow(2.0, pow( 2.0 - exp.x, 2.0) * (-3.0));
	float  factor2 = pow(2.0, pow(exp.x, 2.0) * -3.0); // used for the same stuff as 'factor', just doesn't fit in a vec4 :)

	vec3 power = vec3(0.0,0.0,0.0);
	power.x = pow(2.0, pow(exp.y, 2.0) * -8.0);
	power.y = pow(2.0, pow(-exp.y + -1.0, 2.0) * -8.0);
	power.z = pow(2.0, pow(-exp.y + 1.0, 2.0) * -8.0);

	vec2 viewPos      = floor(texelPos.xy * viewSize.xy) + 0.5;
	float intencityPos  = fract((viewPos.y * 3.0 + viewPos.x) * 0.166667);
	vec3 intencity = vec3(params.intensityR, params.intensityG, params.intensityB);

	vec4 scanlineIntencity = vec4(0.0,0.0,0.0,0.0);
	if (intencityPos < 0.333)
		scanlineIntencity.rgb = intencity.xyz;
	else if (intencityPos < 0.666)
		scanlineIntencity.rgb = intencity.zxy;
	else
		scanlineIntencity.rgb = intencity.yzx;

	vec3 color1  = texture(Source, (floor(size.zw * textureSize.xy   + vec2( 1, -1))   + 0.5)      / textureSize.xy).rgb * factor.y * params.brightness;
    vec3 color2  = texture(Source, (floor(size.zw * textureSize.xy   + vec2(-2,  0))   + 0.5)      / textureSize.xy).rgb * factor.z * params.brightness;
    vec3 color3  = texture(Source, (floor(size.zw * textureSize.xy   + vec2(-1,  0))   + 0.5)      / textureSize.xy).rgb * factor.x * params.brightness;
    vec3 color4  = texture(Source, (floor(size.zw * textureSize.xy   + vec2( 1,  0))   + 0.5)      / textureSize.xy).rgb * factor.y * params.brightness;
    vec3 color5  = texture(Source, (floor(size.zw * textureSize.xy   + 0)                + 0.5)      / textureSize.xy).rgb * factor2  * params.brightness;
    vec3 color6  = texture(Source, (floor(size.zw * textureSize.xy   + vec2(-1,  1))   + 0.5)      / textureSize.xy).rgb * factor.x * params.brightness;
    vec3 color7  = texture(Source, (floor(size.zw * textureSize.xy   + vec2( 2,  0))   + 0.5)      / textureSize.xy).rgb * factor.w * params.brightness;
    vec3 color8  = texture(Source, (floor(size.zw * textureSize.xy   + -1)               + 0.5)      / textureSize.xy).rgb * factor.x * params.brightness;
    vec3 color9  = texture(Source, (floor(size.zw * textureSize.xy   + vec2( 0, -1))   + 0.5)      / textureSize.xy).rgb * factor2  * params.brightness;
    vec3 color10 = texture(Source, (floor(size.zw * textureSize.xy   + 1)                + 0.5)      / textureSize.xy).rgb * factor.y * params.brightness;
    vec3 color11 = texture(Source, (floor(size.xy * textureSize.xy   + vec2( 0,  1))   + 0.5)      / textureSize.xy).rgb * factor2  * params.brightness;

	vec3 final = 
	power.x * (color2 + color3 + color4 + color5 + color7) / (factor.z + factor.x + factor.y + factor2 + factor.w) +
	power.y * (color1 + color8 + color9)                   / (factor.y + factor.x + factor2)                 +
	power.z * (color10 + color6 + color11)                 / (factor.y + factor.x + factor2);

	vec4 outColor = vec4(0.0,0.0,0.0,0.0);
	outColor.xyz = params.viewSizeHD < viewSize.y ? (scanlineIntencity.rgb * final.rgb) : final.rgb;
	outColor.w = 1.0;
	
	FragColor.xyzw = outColor.xyzw;
}