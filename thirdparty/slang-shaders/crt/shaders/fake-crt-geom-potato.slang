#version 450


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    float SIZE;
} params;

// Parameter lines go here:

#pragma parameter SIZE "Mask Type " 2.0 2.00 3.00 1.0
#define SIZE params.SIZE

#define OriginalSize params.OriginalSize
#define OutputSize params.OutputSize
#define SourceSize params.SourceSize

#define pi 3.141592654
	
layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 2) out  float fragpos;


void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.0001;
    fragpos = vTexCoord.x*OutputSize.x*pi*2.0/SIZE;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(location = 2) in  float fragpos;


layout(set = 0, binding = 1) uniform sampler2D Source;

vec2 Warp(vec2 pos)
{
    pos = pos*2.0-1.0;
    pos *= vec2(1.0+pos.y*pos.y*0.03, 1.0+pos.x*pos.x*0.04);
    pos = pos*0.5+0.5;
    return pos;
}

void main()
{
vec2 pos = Warp(vTexCoord);
vec2 corn = min(pos, 1.0-pos);    // This is used to mask the rounded
  corn.x = 0.0002/corn.x;         // corners later on

float y = pos.y*SourceSize.y;
vec2 dx = vec2(SourceSize.z*0.75,0.0);
vec2 dy = vec2(0.0,SourceSize.w*0.5);

// precalculated kaizer window filter
vec3 res = vec3(0.0);
res += texture(Source,pos -dx).rgb*-0.26565;
res += texture(Source,pos -dy).rgb*-0.26565;
res += texture(Source,pos ).rgb*1.5313;


vec3 clean = res;
float w = dot(vec3(0.15),res);

// vignette  
float x = (vTexCoord.x-0.5);  // range -0.5 to 0.5, 0.0 being center of screen
      x = x*x;   
res *= (0.25+x)*sin((y-0.25)*pi*2.0)+(0.75-x);
res *= 0.15*sin(fragpos)+0.85;

res = mix(res,clean, w);

res *= vec3(1.0,0.9,1.15);


float lum = dot(vec3(0.29,0.6,0.11),res);
res = mix(vec3(lum),res, 1.1);

res *= mix(1.25,1.0,w);
if (corn.y <= corn.x || corn.x < 0.0001 )res = vec3(0.0);

FragColor.rgb = res;   
}
