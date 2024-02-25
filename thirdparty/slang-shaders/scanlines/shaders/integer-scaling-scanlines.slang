#version 450
/* 
Simple pixel perfect scanlines for use *only* with interger scaling mode. 
Perfect for low res output and low performance machines.
Also great if you dont want any moire patterns pin sharp scanlines 
Use in combination with HDR mode in latest RetroArch and very bright 
HDR monitors to get 'glowing' scanlines much like the original CRT's but sharper 

Recommended setting for 720p 1 pixel width scalines at 2x or 3x scalings
Recommended setting for 1080p 2 or 3 pixel width scalines at 4x or 5x scalings
Recommended setting for 4K 4 or 5 pixel width scalines at 8x or 9x scalings
*/
layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float ScanlineWidth;
   float ScreenWidth;
   float ScreenHeight; 
} params;

#pragma parameter ScanlineWidth "Scanline Width"  4.0    0.0 20.0   1.0
#pragma parameter ScreenWidth   "Screen Width"    3840.0 0.0 7680.0 1.0
#pragma parameter ScreenHeight  "Screen Height"   2160.0 0.0 4320.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float Scale;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;

   vec2 ScreenSize = max(params.OutputSize.xy, vec2(params.ScreenWidth, params.ScreenHeight));

   if((params.SourceSize.x > ScreenSize.x) || (params.SourceSize.y > ScreenSize.y))
   {
	   Scale = 1.0;
   }
   else 
   {
      float ScaleFactor = 2.0;
        
      while(((params.SourceSize.x * ScaleFactor) <= ScreenSize.x) && ((params.SourceSize.y * ScaleFactor) <= ScreenSize.y))
    	{
	      ScaleFactor += 1.0;
      }

	   Scale = ScaleFactor - 1.0;
   }
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float Scale;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   vec2 InPixels = (vTexCoord * params.SourceSize.xy) * vec2(Scale);

   if(floor(mod(InPixels.y, Scale)) < params.ScanlineWidth)
   {
      FragColor = vec4(texture(Source, vTexCoord).xyz, 1.0);
   }
   else
   {
      FragColor = vec4(0.0,0.0,0.0,1.0);
   }
}
