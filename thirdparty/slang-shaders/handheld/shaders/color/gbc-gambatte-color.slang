#version 450

/*
   Gambatte Color
   A Slang/GLSL port of the color correction option on Gambatte emulator
   Ported by: RiskyJumps
   License: Public domain
*/

/*
OPTIONS:

INT_OPS (default: Disabled)
It's supposed to be more "accurate" but it's a waste. Not recommended
*/
//#define INT_OPS

/*
SIMULATE_INT (default: Disabled)
Only meaningful if INT_OPS is disabled. It truncates floats. Then
again, it's supposed to be more "accurate" but it looks just
too similar. It's still a waste. Not recommended.
*/
//#define SIMULATE_INT

layout(std140, set = 0, binding = 0) uniform UBO
{
     mat4 MVP;
     vec4 OutputSize;
     vec4 OriginalSize;
     vec4 SourceSize;
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
     vec4 color = texture(Source, vTexCoord);

#ifdef INT_OPS
     color.rgb *= 255.0;

     int r = (int)color.r;
     int g = (int)color.g;
     int b = (int)color.b;

     int R = (r * 13 + g * 2 + b) >> 4;
     int G = (g *  3 + b) >> 2;
     int B = (r *  3 + g * 2 + b * 11) >> 4;

     color.rgb = vec3((float)R, (float)G, (float)B);
     color.rgb /= 255.0;

     FragColor = color;
     return;

#else
     mat3 color_correction = mat3(
          13.0,  2.0,   1.0,
           0.0,  3.0,   1.0,
           3.0,  2.0,  11.0
     );

     mat3 scale = mat3(
          1.0/16.0,      0.0,       0.0,
               0.0,  1.0/4.0,       0.0,
               0.0,      0.0,  1.0/16.0
     );

     color_correction *= scale;

#ifdef SIMULATE_INT
     color.rgb *= 255.0;
     color.rgb = floor(color.rgb);
     color.rgb *= color_correction;
     color.rgb = floor(color.rgb);
     color.rgb /= 255.0;
     FragColor = color;
     return;

#else
     color.rgb *= color_correction;
     FragColor = color;
     return;

#endif

#endif
}
