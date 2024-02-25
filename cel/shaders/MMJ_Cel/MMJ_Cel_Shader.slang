#version 450

/*
MMJ's Cel Shader - v1.03
----------------------------------------------------------------
-- 180403 --
This is a port of my old shader from around 2006 for Pete's OGL2 
plugin for ePSXe. It started out as a shader based on the 
"CComic" shader by Maruke. I liked his concept, but I was 
looking for something a little different in the output. 

Since the last release, I've seen some test screenshots from MAME 
using a port of my original shader and have also seen another 
port to get it working with the PCSX2 emulator. Having recently 
seen some Kingdom Hearts II and Soul Calibur 3 YouTube videos with 
my ported shader inspired me to revisit it and get it working in 
RetroArch.

As for this version (1.03), I've made a few small modifications 
(such as to remove the OGL2Param references, which were specific 
to Pete's plugin) and I added some RetroArch Parameter support, 
so some values can now be changed in real time.

Keep in mind, that this was originally developed for PS1, using
various 3D games as a test. In general, it will look better in 
games with less detailed textures, as "busy" textures will lead 
to more outlining / messy appearance. Increasing "Outline 
Brightness" can help mitigate this some by lessening the 
"strength" of the outlines.

Also (in regards to PS1 - I haven't really tested other systems 
too much yet), 1x internal resolution will look terrible. 2x 
will also probably be fairly blurry/messy-looking. For best 
results, you should probably stick to 4x or higher internal 
resolution with this shader.

Parameters:
-----------
White Level Cutoff = Anything above this luminance value will be 
    forced to pure white.

Black Level Cutoff = Anything below this luminance value will be 
    forced to pure black.

Shading Levels = Determines how many color "slices" there should 
    be (not counting black/white cutoffs, which are always 
    applied).

Saturation Modifier = Increase or decrease color saturation. 
    Default value boosts saturation a little for a more 
    cartoonish look. Set to 0.00 for grayscale.

Outline Brightness = Adjusts darkness of the outlines. At a 
    setting of 1, outlines should be disabled.

Shader Strength = Adjusts the weight of the color banding 
    portion of the shader from 0% (0.00) to 100% (1.00). At a 
    setting of 0.00, you can turn off the color banding effect 
    altogether, but still keep outlines enabled.
-----------
MMJuno
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float WhtCutoff;
	float BlkCutoff;
	float ShdLevels;
	float SatModify;
	float OtlModify;
	float ShdWeight;
} params;

#pragma parameter WhtCutoff "White Level Cutoff" 0.97 0.50 1.00 0.01
#pragma parameter BlkCutoff "Black Level Cutoff" 0.03 0.00 0.50 0.01
#pragma parameter ShdLevels "Shading Levels" 9.0 1.0 16.0 1.0
#pragma parameter SatModify "Saturation Modifier" 1.15 0.00 2.00 0.01
#pragma parameter OtlModify "Outline Brightness" 0.20 0.00 1.00 0.01
#pragma parameter ShdWeight "Shader Strength" 0.50 0.00 1.00 0.01

#define WhtCutoff params.WhtCutoff
#define BlkCutoff params.BlkCutoff
#define ShdLevels params.ShdLevels
#define SatModify params.SatModify
#define OtlModify params.OtlModify
#define ShdWeight params.ShdWeight

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 TEX0;
layout(location = 2) out vec4 TEX1;
layout(location = 3) out vec4 TEX2;
layout(location = 4) out vec4 TEX3;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
   
	TEX0 = vTexCoord.xyxy;
    vec4 offset;
    offset.xy = -(offset.zw = vec2(params.SourceSize.z, 0.0));
    TEX1 = TEX0 + offset;
    
    offset.xy = -(offset.zw = vec2(0.0, params.SourceSize.w));
    TEX2 = TEX0 + offset;
    TEX3 = TEX1 + offset;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 TEX0;
layout(location = 2) in vec4 TEX1;
layout(location = 3) in vec4 TEX2;
layout(location = 4) in vec4 TEX3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec3 RGB2HSL(vec3 cRGB) 
{
    float cR = cRGB[0], cG = cRGB[1], cB = cRGB[2];
    float vMin = min(min(cR, cG), cB), vMax = max(max(cR, cG), cB);
    float dMax = vMax - vMin, vS = 0.0, vH = 0.0, vL = (vMax + vMin) / 2.0;

    // gray, no chroma
    if(dMax == 0.0) { 
      vH = 0.0; vS = vH; 
      
    // chromatic data
    } else {
        if(vL < 0.5) { vS = dMax / (vMax + vMin); }
        else         { vS = dMax / (2.0 - vMax - vMin); }

        float dR = (((vMax - cR) * 0.1667) + (dMax * 0.5)) / dMax;
        float dG = (((vMax - cG) * 0.1667) + (dMax * 0.5)) / dMax;
        float dB = (((vMax - cB) * 0.1667) + (dMax * 0.5)) / dMax;

        if     (cR >= vMax) { vH = dB - dG; }
        else if(cG >= vMax) { vH = 0.3333 + dR - dB; }
        else if(cB >= vMax) { vH = 0.6667 + dG - dR; }

        if     (vH < 0.0) { vH += 1.0; }
        else if(vH > 1.0) { vH -= 1.0; }
    }
    return vec3(vH, vS, vL);
}

float Hue2RGB(float v1, float v2, float vH) 
{
    float v3 = 0.0;

    if     (vH < 0.0) { vH += 1.0; }
    else if(vH > 1.0) { vH -= 1.0; }

    if     ((6.0 * vH) < 1.0) { v3 = v1 + (v2 - v1) * 6.0 * vH; }
    else if((2.0 * vH) < 1.0) { v3 = v2; }
    else if((3.0 * vH) < 2.0) { v3 = v1 + (v2 - v1) * (0.6667 - vH) * 6.0; }
    else                      { v3 = v1; }

    return v3;
}

vec3 HSL2RGB(vec3 vHSL) 
{
    float cR = 0.0, cG = cR, cB = cR;

    if(vHSL[1] == 0.0) {
      cR = vHSL[2], cG = cR, cB = cR;

    } else {
        float v1 = 0.0, v2 = v1;

        if(vHSL[2] < 0.5) { v2 = vHSL[2] * (1.0 + vHSL[1] ); }
        else              { v2 = (vHSL[2] + vHSL[1] ) - (vHSL[1] * vHSL[2] ); }

        v1 = 2.0 * vHSL[2] - v2;

        cR = Hue2RGB(v1, v2, vHSL[0] + 0.3333);
        cG = Hue2RGB(v1, v2, vHSL[0] );
        cB = Hue2RGB(v1, v2, vHSL[0] - 0.3333);
    }
    return vec3(cR, cG, cB);
}

vec3 colorAdjust(vec3 cRGB) 
{
    vec3 cHSL = RGB2HSL(cRGB);

    float cr = 1.0 / ShdLevels;

    // brightness modifier
    float BrtModify = mod(cHSL[2], cr); 

    if     (cHSL[2] > WhtCutoff) { cHSL[1]  = 1.0; cHSL[2] = 1.0; }
    else if(cHSL[2] > BlkCutoff) { cHSL[1] *= SatModify; cHSL[2] += (cHSL[2] * cr - BrtModify); }
    else                         { cHSL[1]  = 0.0; cHSL[2] = 0.0; }
    cRGB = 1.2 * HSL2RGB(cHSL);

    return cRGB;
}

void main()
{
	vec3 c0 = texture(Source, TEX3.xy).rgb;
	vec3 c1 = texture(Source, TEX2.xy).rgb;
	vec3 c2 = texture(Source, TEX3.zy).rgb;
	vec3 c3 = texture(Source, TEX1.xy).rgb;
	vec3 c4 = texture(Source, TEX0.xy).rgb;
	vec3 c5 = texture(Source, TEX1.zw).rgb;
	vec3 c6 = texture(Source, TEX3.xw).rgb;
	vec3 c7 = texture(Source, TEX2.zw).rgb;
	vec3 c8 = texture(Source, TEX3.zw).rgb;

	vec3 c9 = ((c0 + c2 + c6 + c8) * 0.15 + (c1 + c3 + c5 + c7) * 0.25 + c4) / 2.6;

	vec3 o = vec3(1.0); vec3 h = vec3(0.05); vec3 hz = h; float k = 0.005; 
	float kz = 0.007; float i = 0.0;

	vec3 cz = (c9 + h) / (dot(o, c9) + k);

	hz = (cz - ((c0 + h) / (dot(o, c0) + k))); i  = kz / (dot(hz, hz) + kz);
	hz = (cz - ((c1 + h) / (dot(o, c1) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c2 + h) / (dot(o, c2) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c3 + h) / (dot(o, c3) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c5 + h) / (dot(o, c5) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c6 + h) / (dot(o, c6) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c7 + h) / (dot(o, c7) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c8 + h) / (dot(o, c8) + k))); i += kz / (dot(hz, hz) + kz);

	i /= 8.0; i = pow(i, 0.75);

	if(i < OtlModify) { i = OtlModify; }
	c9 = min(o, min(c9, c9 + dot(o, c9)));
	FragColor.rgb = mix(c4 * i, colorAdjust(c9 * i), ShdWeight);
}