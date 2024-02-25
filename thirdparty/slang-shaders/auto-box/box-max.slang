#version 450

/*
   Box Max Shader
   Automatically scales output to the maximum integer scale possible.
   Author: Themaister
   License: Public domain
*/

layout(std140, set = 0, binding = 0) uniform UBO
{
   vec4 SourceSize;
   vec4 OriginalSize;
   vec4 OutputSize;
   uint FrameCount;
   mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;


float min(vec2 x)
{
   if (x.x > x.y)
      return x.y;
   else
      return x.x;
}

void main()
{
   gl_Position = global.MVP * Position;

   vec2 box_scale = floor(global.OutputSize.xy / global.SourceSize.xy);
   box_scale = vec2(min(box_scale), min(box_scale));

   vec2 scale = (global.OutputSize.xy / global.SourceSize.xy) / box_scale;
   vec2 middle = vec2(0.5);
   vec2 diff = TexCoord - middle;
   vTexCoord = middle + diff * scale;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   FragColor = vec4(texture(Source, vTexCoord).rgb, 1.0);
}