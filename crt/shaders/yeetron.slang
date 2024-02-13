#version 450

// ported from ReShade

layout(push_constant) uniform Push
{
	float viewSizeHD;
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
	float RSDK_PI = 3.14159;
	vec2 textureSize = params.SourceSize.xy; // Size of Internal Framebuffer
	vec2 viewSize = global.FinalViewportSize.xy; // Size of final Viewport
	vec2 pixelSize = params.OriginalSize.xy; // Size of Game resolution;
	
	vec2 viewPos      = floor((textureSize.xy / pixelSize.xy) * vTexCoord.xy * viewSize.xy) + 0.5;
    float intencityPos  = fract((viewPos.y * 3.0 + viewPos.x) * 0.166667);

	vec3 intencity = vec3(params.intensityR, params.intensityG, params.intensityB);
    vec4 scanlineIntencity = vec4(0.0,0.0,0.0,0.0);
    if (intencityPos < 0.333)
        scanlineIntencity.rgb = intencity.xyz;
    else if (intencityPos < 0.666)
        scanlineIntencity.rgb = intencity.zxy;
    else
        scanlineIntencity.rgb = intencity.yzx;

    vec2 pixelPos         = vTexCoord.xy * textureSize.xy;
    vec2 roundedPixelPos  = floor(pixelPos.xy);

    scanlineIntencity.a = clamp(abs(sin(pixelPos.y * RSDK_PI)) + 0.25, 0.5, 1.0);
    pixelPos.xy         = fract(pixelPos.xy) + -0.5;

    vec2 invTexPos = -vTexCoord.xy * textureSize.xy + (roundedPixelPos + 0.5);
    
    vec2 newTexPos = vec2(0.0,0.0);
    newTexPos.x = clamp(-abs(invTexPos.x * 0.5) + 1.5, 0.8, 1.25);
    newTexPos.y = clamp(-abs(invTexPos.y * 2.0) + 1.25, 0.5, 1.0);

    vec2 colorMod;
    colorMod.x = newTexPos.x * newTexPos.y;
    colorMod.y = newTexPos.x * ((scanlineIntencity.a + newTexPos.y) * 0.5);

    scanlineIntencity.w *= newTexPos.x;

    vec2 texPos   = ((pixelPos.xy + -clamp(pixelPos.xy, -0.25, 0.25)) * 2.0 + roundedPixelPos + 0.5) / textureSize.xy;
    vec4 texColor = texture(Source, texPos.xy);

    vec3 blendedColor = vec3(0.0,0.0,0.0);
    blendedColor.x  = scanlineIntencity.a * texColor.x;
    blendedColor.yz = colorMod.xy * texColor.yz;

    vec4 outColor;
    outColor.xyz    = viewSize.y >= params.viewSizeHD ? (scanlineIntencity.rgb * blendedColor.rgb) : blendedColor.xyz;
    outColor.w      = texColor.w;
	
	FragColor = outColor.xyzw;
}