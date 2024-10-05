#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	uint TotalSubFrames;
	uint CurrentSubFrame;
	vec4 FinalViewportSize;
	float bordersize, bordercolor;
} params;

#pragma parameter bordersize "Border Thickness" 0.02 0.0 1.0 0.01
#pragma parameter bordercolor "Border Color" 1.0 1.0 6.0 1.0
int col = int(params.bordercolor);

const vec4 red = vec4(1.,0.,0.,1.);
const vec4 magenta = vec4(1.,0.,1.,1.);
const vec4 green = vec4(0.,1.,0.,1.);
const vec4 blue = vec4(0.,0.,1.,1.);
const vec4 cyan = vec4(0.,1.,1.,1.);
const vec4 yellow = vec4(1.,1.,0.,1.);

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 sindenBorder;
layout(location = 2) out vec4 borderCol;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   float borderthick = params.bordersize * 0.25;
   // aspect-corrected width
   sindenBorder.x = borderthick * (params.FinalViewportSize.y * params.FinalViewportSize.z);
   sindenBorder.y = 1.-sindenBorder.x;
   sindenBorder.z = borderthick;
   sindenBorder.w = 1.-borderthick;
   if(col == 2) borderCol = yellow;
   else if(col == 3) borderCol = green;
   else if(col == 4) borderCol = cyan;
   else if(col == 5) borderCol = blue;
   else if(col == 6) borderCol = magenta;
   else borderCol = red;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 sindenBorder;
layout(location = 2) in vec4 borderCol;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	FragColor = (vTexCoord.x < sindenBorder.x || vTexCoord.x > sindenBorder.y || vTexCoord.y < sindenBorder.z || vTexCoord.y > sindenBorder.w) ? borderCol : texture(Source, vTexCoord);
}
