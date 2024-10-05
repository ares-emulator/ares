#version 450

/*
   Trilinear LUT
   Slightly more accurate, but slower, than reshade/shaders/LUT/LUT.slang
   Compatible with the images in that directory, or this one.
   Expects n^2 x n dimensions with green on the vertical axis, red on the minor horizontal axis, blue on the major horizontal axis.
   Linear filtering for the LUT image should be FALSE. (Reshade's LUT expects true.)
   
   Copyright (C) 2023 ChthonVII

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float lut_toggle, lut_index, lut_chroma_adapt;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter lut_toggle "Gamut: sRGB, P22-80s, P22-90s, P22-J, Trinitron P22" 4 0 4 1
#pragma parameter lut_index "White point: SMPTE-C (D65) | NTSC-U (C) | NTSC-J (9300K+27MPCD)" 1 0 2 1
#pragma parameter lut_chroma_adapt "(For P22-J and Trinitron w/ NTSC-J) Chromatic Adaptation to D65" 0 0 1 1

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   //vTexCoord = TexCoord * 1.0001;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PhosphorSamplerLUT1;
layout(set = 0, binding = 4) uniform sampler2D PhosphorSamplerLUT2;
layout(set = 0, binding = 5) uniform sampler2D PhosphorSamplerLUT3;
layout(set = 0, binding = 6) uniform sampler2D PhosphorSamplerLUT4;
layout(set = 0, binding = 7) uniform sampler2D PhosphorSamplerLUT5;
layout(set = 0, binding = 8) uniform sampler2D PhosphorSamplerLUT6;

#define saturate(c) clamp(c, 0.0, 1.0)

void main()
{
	vec2 LUT_Size = textureSize(PhosphorSamplerLUT1, 0); // All are the same size... right?
	vec3 imgColor = texture(Source, vTexCoord.xy).rgb;
	
	// Chromatic adaptation is available for P22-J and Trinitron P22 when using white D93.
	bool doChromaAdapt = params.lut_toggle > 2.5 && params.lut_index > 1.5 && params.lut_chroma_adapt > 0.5;
	
	// Force disable chromatic adaptation if a chroma-adapted LUT is missing.
	if(!doChromaAdapt) {
	    // Change the white balance ourselves, and use the D65-to-D65 LUT.
	    
	    float Wx, Wy;
	    if(params.lut_index < 0.5) { 
            // D65
            Wx = 0.31266142;
            Wy = 0.3289589;
        } else if(params.lut_index < 1.5) {
            Wx = 0.31; // C
            Wy = 0.316;
        } else {
            // grade.slang D93
            // I do not know where this originates from, but it was in Dogway's grading shader.
            // It is not the same 9300K as the lookup table from gamutthingy.
            Wx = 0.281;
            Wy = 0.311;
        }
        
        vec3 d65 = vec3(0.31266142, 0.3289589, 1 - 0.31266142 - 0.3289589) / 0.3289589;
        vec3 c = vec3(Wx, Wy, 1 - Wx - Wy) / Wy;
	    
	    mat3 primaries;
	    if(params.lut_toggle < 0.5) {
	        primaries = mat3(0.64, 0.30, 0.15, // sRGB phosphors
                             0.33, 0.60, 0.06,
                             0.03, 0.10, 0.79);
	    } else if(params.lut_toggle < 1.5) {
	        primaries = mat3( // P22-80s, from Grade
                 0.6470, 0.2820, 0.1472,
                 0.3430, 0.6200, 0.0642,
                 0.0100, 0.0980, 0.7886);
	    } else if(params.lut_toggle < 2.5) {
	        primaries = mat3( // P22-90s, from Grade
                 0.6661, 0.3134, 0.1472,
                 0.3329, 0.6310, 0.0642,
                 0.0010, 0.0556, 0.7886);
        } else if(params.lut_toggle < 3.5) {
            primaries = mat3( // P22-J, from Grade
                 0.625, 0.280, 0.152,
                 0.350, 0.605, 0.062,
                 0.025, 0.115, 0.786);
	    } else {
	        primaries = mat3(0.621, 0.281, 0.152, // Trinitron phosphors
                             0.330, 0.606, 0.067,
                             0.049, 0.113, 0.781);
	    }
        
        mat3 unbalanced = mat3(primaries[0][0] / primaries[1][0], 1.0, primaries[2][0] / primaries[1][0],
                           primaries[0][1] / primaries[1][1], 1.0, primaries[2][1] / primaries[1][1],
                           primaries[0][2] / primaries[1][2], 1.0, primaries[2][2] / primaries[1][2]);
        mat3 unbInv = inverse(unbalanced);
        vec3 d65NormConsts = unbInv * d65;
        vec3 cNormConsts = unbInv * c;
        vec3 normConsts = cNormConsts / d65NormConsts;
        normConsts /= max(normConsts.r, max(normConsts.g, normConsts.b));
        imgColor *= normConsts; // Scale R, G, and B to get the white point that we want.
	}
	
	if(params.lut_toggle < 0.5) {
	    FragColor = vec4(imgColor, 1.0);
	    return;
	}
		
	// find rgb values and their ceilings and floors on a scale on 0-63
	vec3 temp = imgColor.rgb  * (vec3(LUT_Size.y - 1.0));
	vec3 floors = floor(imgColor.rgb  * (vec3(LUT_Size.y - 1.0)));
	vec3 ceils = ceil(imgColor.rgb  * (vec3(LUT_Size.y - 1.0)));
	
	// how close are we to the ceiling on a scale of 0-1?
	vec3 weights = saturate(temp - floors);
	
	// retroarch can't correctly sample a 1.0 coordinate
	// so we are going to add a just-under-half-step offset to red and green, then increase their divisors by 1
	// This should get us a slightly lower coordinate within the same pixel
	// This is where reshade's LUT is slightly inaccurate because it's taking a linearly filtered sample from the wrong spot.
	floors = floors + vec3(0.4999, 0.4999, 0.0);
	ceils = ceils + vec3(0.4999, 0.4999, 0.0);
	floors = floors / vec3((LUT_Size.y * LUT_Size.y), LUT_Size.y, LUT_Size.y);
	ceils = ceils / vec3((LUT_Size.y * LUT_Size.y), LUT_Size.y, LUT_Size.y);
	
	// this would be correct, if retroarch sampled correctly.
	//floors = saturate(floors / vec3(((LUT_Size.y * LUT_Size.y) - 1.0), LUT_Size.y - 1.0, LUT_Size.y));
	//ceils = saturate(ceils / vec3(((LUT_Size.y * LUT_Size.y) - 1.0), LUT_Size.y - 1.0, LUT_Size.y));
	
	// take 8 samples
	vec3 RfGfBf, RfGfBc, RfGcBf, RfGcBc, RcGfBf, RcGfBc, RcGcBf, RcGcBc;
	
#define eightSamples(SamplerLUT) \
    RfGfBf = (texture(SamplerLUT, vec2(floors.b + floors.r, floors.g))).xyz; \
    RfGfBc = (texture(SamplerLUT, vec2(ceils.b + floors.r, floors.g))).xyz; \
    RfGcBf = (texture(SamplerLUT, vec2(floors.b + floors.r, ceils.g))).xyz; \
    RfGcBc = (texture(SamplerLUT, vec2(ceils.b + floors.r, ceils.g))).xyz; \
    RcGfBf = (texture(SamplerLUT, vec2(floors.b + ceils.r, floors.g))).xyz; \
    RcGfBc = (texture(SamplerLUT, vec2(ceils.b + ceils.r, floors.g))).xyz; \
    RcGcBf = (texture(SamplerLUT, vec2(floors.b + ceils.r, ceils.g))).xyz; \
    RcGcBc = (texture(SamplerLUT, vec2(ceils.b + ceils.r, ceils.g))).xyz;
	
	if(doChromaAdapt) {
	    if(params.lut_toggle < 3.5) {
	        eightSamples(PhosphorSamplerLUT5);
        } else {
            eightSamples(PhosphorSamplerLUT6);
        }
    } else {
	    if(params.lut_toggle < 1.5) {
	        eightSamples(PhosphorSamplerLUT1);
	    } else if(params.lut_toggle < 2.5) {
	        eightSamples(PhosphorSamplerLUT2);
	    } else if (params.lut_toggle < 3.5) {
	        eightSamples(PhosphorSamplerLUT3);
	    } else {	    
	        eightSamples(PhosphorSamplerLUT4);
	    }
    }

#undef eightSamples
	
	// merge down to 4 samples along blue axis
	vec3 RfGf = saturate(mix(RfGfBf, RfGfBc, vec3(weights.b)));
	vec3 RfGc = saturate(mix(RfGcBf, RfGcBc, vec3(weights.b)));
	vec3 RcGf = saturate(mix(RcGfBf, RcGfBc, vec3(weights.b)));
	vec3 RcGc = saturate(mix(RcGcBf, RcGcBc, vec3(weights.b)));
	
	// merge down to 2 samples along green axis
	vec3 Rf = saturate(mix(RfGf, RfGc, vec3(weights.g)));
	vec3 Rc = saturate(mix(RcGf, RcGc, vec3(weights.g)));
	
	// merge down to one color along red axis
	vec3 finalcolor = saturate(mix(Rf, Rc, vec3(weights.r)));
	
	// output
	FragColor = vec4(finalcolor.rgb, 1.0);

}
