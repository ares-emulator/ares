#version 450
// Ribbon Assault -  2014-02-26
// https://www.shadertoy.com/view/MdBGDK

// Inspired by the 'Kali2 scope' - https://www.shadertoy.com/view/lsBGWK
// Should be fast full-screen for everybody. : )
// Use mouse for manual movement.

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
const vec2 madd = vec2(0.5, 0.5);
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = gl_Position.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
float iGlobalTime = float(global.FrameCount)*0.025;
vec2 iResolution = global.OutputSize.xy;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float gTime = iGlobalTime+11.0;

    float f = 3., g = 3.;
	vec2 res = iResolution.xy;
#ifdef MOUSE
	vec2 mou = iMouse.xy;
	if (iMouse.z < 0.5)
	{
		mou = vec2(sin(gTime * .3)*sin(gTime * .17) * 1. + sin(gTime * .3),(1.0-cos(gTime * .632))*sin(gTime * .131)*1.0+cos(gTime * .3));
		mou = (mou+1.0) * res;
	}
#else
   vec2 mou = vec2(0.0, 0.0);
   mou = vec2(sin(gTime * .3)*sin(gTime * .17) * 1. + sin(gTime * .3),(1.0-cos(gTime * .632))*sin(gTime * .131)*1.0+cos(gTime * .3));
   mou = (mou+1.0) * res;
#endif
	vec2 z = ((-res+2.0 * fragCoord.xy) / res.y);
	vec2 p = ((-res+2.0+mou) / res.y);
	for( int i = 0; i < 20; i++) 
	{
		float d = dot(z,z);
		z = (vec2( z.x, -z.y ) / d) + p; 
		z.x =  abs(z.x);
		f = max( f, (dot(z-p,z-p) ));
		g = min( g, sin(dot(z+p,z+p))+1.0);
	}
	f = abs(-log(f) / 3.5);
	g = abs(-log(g) / 8.0);
	fragColor = vec4(min(vec3(g, g*f, f), 1.0),1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
