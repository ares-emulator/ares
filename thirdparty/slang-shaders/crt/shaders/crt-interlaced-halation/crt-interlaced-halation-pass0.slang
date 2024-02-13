#version 450

/*
    CRT-interlaced-halation shader - pass0

    Like the CRT-interlaced shader, but adds a subtle glow around bright areas
    of the screen.

    Copyright (C) 2010-2012 cgwg, Themaister and DOLLS

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have the original version of this shader
    distributed under the GPL in this message:

        http://board.byuu.org/viewtopic.php?p=26075#p26075

        "Feel free to distribute my shaders under the GPL. After all, the
        barrel distortion code was taken from the Curvature shader, which is
        under the GPL."
    )
*/

        #define CRTgamma 2.5
        #define display_gamma 2.2
        #define TEX2D(c) pow(texture(Source,(c)),vec4(CRTgamma))

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
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
   float dy = params.SourceSize.w;
   
    t1 = vTexCoord.xyyy + vec4( 0, -4.0*dy, -3.0*dy, -2.0*dy);
    t2 = vTexCoord.xyyy + vec4( 0,     -dy,       0,      dy);
    t3 = vTexCoord.xyyy + vec4( 0,  2.0*dy,  3.0*dy,  4.0*dy);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
                float wid = 2.0;

                float c1 = exp(-1.0/wid/wid);
                float c2 = exp(-4.0/wid/wid);
                float c3 = exp(-9.0/wid/wid);
                float c4 = exp(-16.0/wid/wid);
                float norm = 1.0 / (1.0 + 2.0*(c1+c2+c3+c4));

                vec4 sum = vec4(0.0, 0.0, 0.0, 0.0);

                sum += TEX2D(t1.xy) * vec4(c4);
                sum += TEX2D(t1.xz) * vec4(c3);
                sum += TEX2D(t1.xw) * vec4(c2);
                sum += TEX2D(t2.xy) * vec4(c1);
                sum += TEX2D(vTexCoord);
                sum += TEX2D(t2.xw) * vec4(c1);
                sum += TEX2D(t3.xy) * vec4(c2);
                sum += TEX2D(t3.xz) * vec4(c3);
                sum += TEX2D(t3.xw) * vec4(c4);

                FragColor = vec4(pow(sum*vec4(norm), vec4(1.0/display_gamma)));
}