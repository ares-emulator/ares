#version 450

/* Filename: chromaticity 

   Copyright (C) 2023 W. M. Martinez
   splitted and adjusted by DariusG

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>. 
   */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    float COLOR_MODE, Dx;
} params;
#pragma parameter COLOR_MODE "SRGB,SMPTE C,REC709,BT2020,SMPTE240,NTSC1953,EBU" 0.0 0.0 6.0 1.0   
#pragma parameter Dx "Color Temp: D50, D55, D65, D75" 2.0 -1.0 3.0 1.0

#define Dx params.Dx
#define COLOR_MODE params.COLOR_MODE

#define WHITE  vec3(1.0, 1.0, 1.0)

/* 
SMPTE-C/170M used by NTSC and PAL and by SDTV in general.
REC709       used by HDTV in general.
SRGB         used by most webcams and computer graphics. ***NOTE***: Gamma 2.4
BT2020       used by Ultra-high definition television (UHDTV) and wide color gamut.
SMPTE240     used during the early days of HDTV (1988-1998).
NTSC1953     used by NTSC at 1953. 
EBU          used by PAL/SECAM in 1975. Identical to REC601.
*/


//               RX   RY      GX     GY     BX      BY      RL    GL    BL     TR0    TR   TR2
// SMPTE C  0.630 0.340 / 0.310 0.595 / 0.155   0.070 / 0.299 0.587 0.114 / 0.018 0.099 4.5
// REC709   0.640 0.330 / 0.300 0.600 / 0.150   0.060 / 0.212 0.715 0.072 / 0.018 0.099 4.5
// SRGB     0.640 0.330 / 0.300 0.600 / 0.150   0.060 / 0.299 0.587 0.114 / 0.040 0.055 12.92
// BT2020   0.708 0.292 / 0.170 0.797 / 0.131   0.046 / 0.262 0.678 0.059 / 0.059 0.099 4.5
// SMPTE240 0.630 0.340 / 0.310 0.595 / 0.155   0.070 / 0.212 0.701 0.086 / 0.091 0.111 4.0
// NTSC1953 0.670 0.330 / 0.210 0.710 / 0.140   0.080 / 0.210 0.710 0.080 / 0.081 0.099 4.5
// EBU      0.640 0.330 / 0.290 0.600 / 0.150   0.060 / 0.299 0.587 0.114 / 0.081 0.099 4.5
// SECAM    0.640 0.330 / 0.290 0.600 / 0.150   0.060 / 0.334 0.585 0.081 / 0.081 0.099 4.5



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
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


mat3 XYZ_TO_sRGB = mat3(
     3.2406255, -0.9689307,  0.0557101,
    -1.5372080,  1.8758561, -0.2040211,
    -0.4986286,  0.0415175,  1.0569959);

mat3 colorspace_rgb()
{
    return XYZ_TO_sRGB;
}


vec3 xyY_to_XYZ(const vec3 xyY)
{
    float x = xyY.x;
    float y = xyY.y;
    float Y = xyY.z;
    float z = 1.0 - x - y;

    return vec3(Y * x / y, Y, Y * z / y);
}



vec3 Yrgb_to_RGB(mat3 toRGB, vec3 W, vec3 Yrgb)
{

//0 SRGB     0.640 0.330 / 0.300 0.600 / 0.150  0.060 --
//1 SMPTE C  0.630 0.340 / 0.310 0.595 / 0.155  0.070 --
//2 REC709   0.640 0.330 / 0.300 0.600 / 0.150  0.060 --
//3 BT2020   0.708 0.292 / 0.170 0.797 / 0.131   0.046 --
//4 SMPTE240 0.630 0.340 / 0.310 0.595 / 0.155   0.070 --
//5 NTSC1953 0.670 0.330 / 0.210 0.710 / 0.140   0.080 
//6 EBU      0.640 0.330 / 0.290 0.600 / 0.150   0.060 --

float CHROMA_A_X, CHROMA_A_Y,CHROMA_B_X, CHROMA_B_Y, CHROMA_C_X, CHROMA_C_Y;
    if (COLOR_MODE == 0.0 || COLOR_MODE == 2.0 ) 
    {
CHROMA_A_X=0.64;
CHROMA_A_Y=0.33;
CHROMA_B_X=0.3;
CHROMA_B_Y=0.6;
CHROMA_C_X= 0.15;
CHROMA_C_Y= 0.06;
    }

else if (COLOR_MODE == 1.0 || COLOR_MODE == 4.0) 
    {
CHROMA_A_X=0.63;
CHROMA_A_Y=0.34;
CHROMA_B_X=0.31;
CHROMA_B_Y=0.595;
CHROMA_C_X= 0.155;
CHROMA_C_Y= 0.070;
    }

else if (COLOR_MODE == 3.0 ) 
    {
CHROMA_A_X=0.708;
CHROMA_A_Y=0.292;
CHROMA_B_X=0.17;
CHROMA_B_Y=0.797;
CHROMA_C_X= 0.131;
CHROMA_C_Y= 0.046;
    }
else if (COLOR_MODE == 5.0 ) 
    {
CHROMA_A_X=0.67;
CHROMA_A_Y=0.33;
CHROMA_B_X=0.21;
CHROMA_B_Y=0.71;
CHROMA_C_X= 0.14;
CHROMA_C_Y= 0.08;
    }

else if (COLOR_MODE == 6.0)
{
CHROMA_A_X=0.64;
CHROMA_A_Y=0.33;
CHROMA_B_X=0.29;
CHROMA_B_Y=0.60;
CHROMA_C_X= 0.15;
CHROMA_C_Y= 0.06;   
}

    mat3 xyYrgb = mat3(CHROMA_A_X, CHROMA_A_Y, Yrgb.r,
                       CHROMA_B_X, CHROMA_B_Y, Yrgb.g,
                       CHROMA_C_X, CHROMA_C_Y, Yrgb.b);
    mat3 XYZrgb = mat3(xyY_to_XYZ(xyYrgb[0]),
                       xyY_to_XYZ(xyYrgb[1]),
                       xyY_to_XYZ(xyYrgb[2]));
    mat3 RGBrgb = mat3(toRGB * XYZrgb[0],
                       toRGB * XYZrgb[1],
                       toRGB * XYZrgb[2]);
    return vec3(dot(W, vec3(RGBrgb[0].r, RGBrgb[1].r, RGBrgb[2].r)),
                dot(W, vec3(RGBrgb[0].g, RGBrgb[1].g, RGBrgb[2].g)),
                dot(W, vec3(RGBrgb[0].b, RGBrgb[1].b, RGBrgb[2].b)));
}

vec3 luminance()
{
//0 SRGB     0.299 0.587 0.114
//1 SMPTE C  0.299 0.587 0.114 
//2 REC709   0.212 0.715 0.072 
//3 BT2020    0.262 0.678 0.059 
//4 SMPTE240  0.212 0.701 0.086 
//5 NTSC1953  0.299 0.587 0.114 
//6 EBU       0.299 0.587 0.114 

 float CHROMA_A_WEIGHT, CHROMA_B_WEIGHT, CHROMA_C_WEIGHT;
 if (COLOR_MODE == 0.0 || COLOR_MODE == 1.0 || 
      COLOR_MODE == 5.0 || COLOR_MODE == 6.0)
{
    CHROMA_A_WEIGHT = 0.299;
    CHROMA_B_WEIGHT = 0.587;
    CHROMA_C_WEIGHT = 0.114;
}

else if (COLOR_MODE == 2.0 || COLOR_MODE == 4.0)
{
    CHROMA_A_WEIGHT = 0.2126;
    CHROMA_B_WEIGHT = 0.7152;
    CHROMA_C_WEIGHT = 0.0722;
}

else if (COLOR_MODE == 3.0 )
{
    CHROMA_A_WEIGHT = 0.2627;
    CHROMA_B_WEIGHT = 0.678;
    CHROMA_C_WEIGHT = 0.0593;
}


    return vec3(CHROMA_A_WEIGHT, CHROMA_B_WEIGHT, CHROMA_C_WEIGHT);
}


//////////////////////////////////////////////// 
/// GAMMA IN FUNCTION /////////////////////////

float sdr_linear(const float x)
{
//            RX   RY      GX     GY     BX      BY      RL    GL    BL       TR1    TR2   TR3
//0 SRGB     0.040 0.055 12.92
//1 SMPTE C  0.018 0.099 4.5
//2 REC709   0.018 0.099 4.5
//3 BT2020   0.059 0.099 4.5
//4 SMPTE240 0.091 0.111 4.0
//5 NTSC1953 0.018 0.099 4.5
//6 EBU      0.081 0.099 4.5

float CRT_TR1 ,CRT_TR2, CRT_TR3, GAMMAIN;

if (COLOR_MODE == 0.0)
{
    CRT_TR1 = 0.04045;
    CRT_TR2 = 0.055;
    CRT_TR3 = 12.92;
    GAMMAIN = 2.4;
}

else if (COLOR_MODE == 1.0 || COLOR_MODE == 2.0)
{
    CRT_TR1 = 0.081;
    CRT_TR2 = 0.099;
    CRT_TR3 = 4.5;
    GAMMAIN = 2.2;
}
else if (COLOR_MODE == 3.0 )
{
    CRT_TR1 = 0.018;
    CRT_TR2 = 0.099;
    CRT_TR3 = 4.5;
    GAMMAIN = 2.2;
}
else if (COLOR_MODE == 4.0 )
{
    CRT_TR1 = 0.0913;
    CRT_TR2 = 0.1115;
    CRT_TR3 = 4.0;
    GAMMAIN = 2.2;
}
else if (COLOR_MODE == 5.0 || COLOR_MODE == 6.0)
{
    CRT_TR1 = 0.081;
    CRT_TR2 = 0.099;
    CRT_TR3 = 4.5;
    GAMMAIN = 2.2;
}

    return x < CRT_TR1 ? x / CRT_TR3 : pow((x + CRT_TR2) / (1.0+ CRT_TR2), GAMMAIN);
}

vec3 sdr_linear(const vec3 x)
{
    return vec3(sdr_linear(x.r), sdr_linear(x.g), sdr_linear(x.b));
}


//////////////////////////////////////////////// 
/// GAMMA OUT FUNCTION /////////////////////////

float srgb_gamma(const float x)
{
//0 SRGB     0.00313 0.055 12.92
//1 SMPTE C  0.018 0.099 4.5
//2 REC709   0.018 0.099 4.5
//3 BT2020   0.059 0.099 4.5
//4 SMPTE240 0.091 0.111 4.0
//5 NTSC1953 0.018 0.099 4.5
//6 EBU      0.081 0.099 4.5

float LCD_TR1 ,LCD_TR2, LCD_TR3, GAMMAOUT;

if (COLOR_MODE == 0.0)
{
    LCD_TR1 = 0.00313;
    LCD_TR2 = 0.055;
    LCD_TR3 = 12.92;
    GAMMAOUT = 2.4;
}

else if (COLOR_MODE == 1.0 || COLOR_MODE == 2.0)
{
    LCD_TR1 = 0.018;
    LCD_TR2 = 0.099;
    LCD_TR3 = 4.5;
    GAMMAOUT = 2.2;
}
else if (COLOR_MODE == 3.0 )
{
    LCD_TR1 = 0.018;
    LCD_TR2 = 0.099;
    LCD_TR3 = 4.5;
    GAMMAOUT = 2.2;
}
else if (COLOR_MODE == 4.0 )
{
    LCD_TR1 = 0.0228;
    LCD_TR2 = 0.1115;
    LCD_TR3 = 4.0;
    GAMMAOUT = 2.2;
}
else if (COLOR_MODE == 5.0 || COLOR_MODE == 6.0)
{
    LCD_TR1 = 0.018;
    LCD_TR2 = 0.099;
    LCD_TR3 = 4.5;
    GAMMAOUT = 2.2;
}



    return x <= LCD_TR1 ? LCD_TR3 * x : (1.0+LCD_TR2) * pow(x, 1.0 / GAMMAOUT) - LCD_TR2;
}

vec3 srgb_gamma(const vec3 x)
{
    return vec3(srgb_gamma(x.r), srgb_gamma(x.g), srgb_gamma(x.b));
}

vec3 TEMP ()
{
    if (Dx == 0.0) return vec3(0.964,1.0,0.8252);
    else if (Dx == 1.0) return vec3(0.95682,1.0,0.92149);
    else if (Dx == 2.0) return vec3(0.95047,1.0,1.0888);
    else if (Dx == 3.0) return vec3(0.94972,1.0,1.22638);
    else return vec3(1.0);
}


void main()
{
    mat3 toRGB = colorspace_rgb();
    vec3 Yrgb = texture(Source, vTexCoord).rgb;
    Yrgb = sdr_linear(Yrgb);
    vec3 W = luminance();
    vec3 RGB = Yrgb_to_RGB(toRGB, W, Yrgb);
    
    RGB = clamp(RGB, 0.0, 1.0);
    RGB = srgb_gamma(RGB)*TEMP();
    FragColor = vec4(RGB, 1.0);
}
