#version 450
// Super Mario Bros. by HLorenzi

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

// Uncomment for totally random level!
// Just to show off Mario's dynamic movements :P
//#define TOTALLY_RANDOM_LEVEL 1

	
// Positions to start and end Mario simulation (relative to screen position)
// You can try changing these! (the difference between the two should be a multiple of 4)
// Put startX too close to endX, and Mario'll experience jittering!
#define startX  0.0
#define endX   80.0







#define RGB(r,g,b) vec4(float(r)/255.0,float(g)/255.0,float(b)/255.0,1.0)

#define SPRROW(x,a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p) (x <= 7 ? SPRROW_H(a,b,c,d,e,f,g,h) : SPRROW_H(i,j,k,l,m,n,o,p))
#define SPRROW_H(a,b,c,d,e,f,g,h) (a+4.0*(b+4.0*(c+4.0*(d+4.0*(e+4.0*(f+4.0*(g+4.0*(h))))))))
#define SECROW(x,a,b,c,d,e,f,g,h) (x <= 3 ? SECROW_H(a,b,c,d) : SECROW_H(e,f,g,h))
#define SECROW_H(a,b,c,d) (a+8.0*(b+8.0*(c+8.0*(d))))
#define SELECT(x,i) mod(floor(i/pow(4.0,float(x))),4.0)
#define SELECTSEC(x,i) mod(floor(i/pow(8.0,float(x))),8.0)

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec4 sprGround(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,1.,0.,0.,0.,0.,0.,0.,0., 0.,2.,1.,0.,0.,0.,0.,1.);
	if (y == 14) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,1.,1.,1.,1.,2.);
	if (y == 13) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,1.,1.,1.,1.,2.);
	if (y == 12) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,1.,1.,1.,1.,2.);
	if (y == 11) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,2.,1.,1.,1.,2.);
	if (y == 10) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,1.,2.,2.,2.,2.,1.);
	if (y ==  9) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,0.,0.,0.,0.,2.);
	if (y ==  8) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,1.,1.,1.,1.,2.);
	
	if (y ==  7) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,1.,1.,1.,1.,2.);
	if (y ==  6) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,2.,0.,1.,1.,1.,1.,2.);
	if (y ==  5) col = SPRROW(x,2.,2.,1.,1.,1.,1.,1.,1., 2.,0.,1.,1.,1.,1.,1.,2.);
	if (y ==  4) col = SPRROW(x,0.,0.,2.,2.,1.,1.,1.,1., 2.,0.,1.,1.,1.,1.,1.,2.);
	if (y ==  3) col = SPRROW(x,0.,1.,0.,0.,2.,2.,2.,2., 0.,1.,1.,1.,1.,1.,1.,2.);
	if (y ==  2) col = SPRROW(x,0.,1.,1.,1.,0.,0.,0.,2., 0.,1.,1.,1.,1.,1.,1.,2.);
	if (y ==  1) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,2., 0.,1.,1.,1.,1.,1.,2.,2.);
	if (y ==  0) col = SPRROW(x,1.,2.,2.,2.,2.,2.,2.,1., 0.,2.,2.,2.,2.,2.,2.,1.);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(247,214,181);
	if (col == 1.0) return RGB(231,90,16);
	return RGB(0,0,0);
}

vec4 sprQuestionBlock(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,3.,0.,0.,0.,0.,0.,0.,0., 0.,0.,0.,0.,0.,0.,0.,3.);
	if (y == 14) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,2.);
	if (y == 13) col = SPRROW(x,0.,1.,2.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,2.,1.,2.);
	if (y == 12) col = SPRROW(x,0.,1.,1.,1.,1.,0.,0.,0., 0.,0.,1.,1.,1.,1.,1.,2.);
	if (y == 11) col = SPRROW(x,0.,1.,1.,1.,0.,0.,2.,2., 2.,0.,0.,1.,1.,1.,1.,2.);
	if (y == 10) col = SPRROW(x,0.,1.,1.,1.,0.,0.,2.,1., 1.,0.,0.,2.,1.,1.,1.,2.);
	if (y ==  9) col = SPRROW(x,0.,1.,1.,1.,0.,0.,2.,1., 1.,0.,0.,2.,1.,1.,1.,2.);
	if (y ==  8) col = SPRROW(x,0.,1.,1.,1.,1.,2.,2.,1., 0.,0.,0.,2.,1.,1.,1.,2.);
	
	if (y ==  7) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,0., 0.,2.,2.,2.,1.,1.,1.,2.);
	if (y ==  6) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,0., 0.,2.,1.,1.,1.,1.,1.,2.);
	if (y ==  5) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 2.,2.,1.,1.,1.,1.,1.,2.);
	if (y ==  4) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,0., 0.,0.,1.,1.,1.,1.,1.,2.);
	if (y ==  3) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,0., 0.,2.,1.,1.,1.,1.,1.,2.);
	if (y ==  2) col = SPRROW(x,0.,1.,2.,1.,1.,1.,1.,1., 2.,2.,1.,1.,1.,2.,1.,2.);
	if (y ==  1) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,2.);
	if (y ==  0) col = SPRROW(x,2.,2.,2.,2.,2.,2.,2.,2., 2.,2.,2.,2.,2.,2.,2.,2.);
	
	if (y < 0 || y > 15) return RGB(107,140,255);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(231,90,16);
	if (col == 1.0) return RGB(255,165,66);
	if (col == 2.0) return RGB(0,0,0);
	return RGB(107,140,255);
}

vec4 sprUsedBlock(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,3.,0.,0.,0.,0.,0.,0.,0., 0.,0.,0.,0.,0.,0.,0.,3.);
	if (y == 14) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y == 13) col = SPRROW(x,0.,1.,0.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,0.,1.,0.);
	if (y == 12) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y == 11) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y == 10) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  9) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  8) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	
	if (y ==  7) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  6) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  5) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  4) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  3) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  2) col = SPRROW(x,0.,1.,0.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,0.,1.,0.);
	if (y ==  1) col = SPRROW(x,0.,1.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,1.,0.);
	if (y ==  0) col = SPRROW(x,3.,0.,0.,0.,0.,0.,0.,0., 0.,0.,0.,0.,0.,0.,0.,3.);
	
	if (y < 0 || y > 15) return RGB(107,140,255);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(0,0,0);
	if (col == 1.0) return RGB(231,90,16);
	return RGB(107,140,255);
}

vec4 sprMarioJump(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,0.,0.,0.,0.,0.,0.,0.,0., 0.,0.,0.,0.,0.,2.,2.,2.);
	if (y == 14) col = SPRROW(x,0.,0.,0.,0.,0.,0.,1.,1., 1.,1.,1.,0.,0.,2.,2.,2.);
	if (y == 13) col = SPRROW(x,0.,0.,0.,0.,0.,1.,1.,1., 1.,1.,1.,1.,1.,1.,2.,2.);
	if (y == 12) col = SPRROW(x,0.,0.,0.,0.,0.,3.,3.,3., 2.,2.,3.,2.,0.,3.,3.,3.);
	if (y == 11) col = SPRROW(x,0.,0.,0.,0.,3.,2.,3.,2., 2.,2.,3.,2.,2.,3.,3.,3.);
	if (y == 10) col = SPRROW(x,0.,0.,0.,0.,3.,2.,3.,3., 2.,2.,2.,3.,2.,2.,2.,3.);
	if (y ==  9) col = SPRROW(x,0.,0.,0.,0.,3.,3.,2.,2., 2.,2.,3.,3.,3.,3.,3.,0.);
	if (y ==  8) col = SPRROW(x,0.,0.,0.,0.,0.,0.,2.,2., 2.,2.,2.,2.,2.,3.,0.,0.);
	
	if (y ==  7) col = SPRROW(x,0.,0.,3.,3.,3.,3.,3.,1., 3.,3.,3.,1.,3.,0.,0.,0.);
	if (y ==  6) col = SPRROW(x,0.,3.,3.,3.,3.,3.,3.,3., 1.,3.,3.,3.,1.,0.,0.,3.);
	if (y ==  5) col = SPRROW(x,2.,2.,3.,3.,3.,3.,3.,3., 1.,1.,1.,1.,1.,0.,0.,3.);
	if (y ==  4) col = SPRROW(x,2.,2.,2.,0.,1.,1.,3.,1., 1.,2.,1.,1.,2.,1.,3.,3.);
	if (y ==  3) col = SPRROW(x,0.,2.,0.,3.,1.,1.,1.,1., 1.,1.,1.,1.,1.,1.,3.,3.);
	if (y ==  2) col = SPRROW(x,0.,0.,3.,3.,3.,1.,1.,1., 1.,1.,1.,1.,1.,1.,3.,3.);
	if (y ==  1) col = SPRROW(x,0.,3.,3.,3.,1.,1.,1.,1., 1.,1.,1.,0.,0.,0.,0.,0.);
	if (y ==  0) col = SPRROW(x,0.,3.,0.,0.,1.,1.,1.,1., 0.,0.,0.,0.,0.,0.,0.,0.);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(0,0,0);
	if (col == 1.0) return RGB(177,52,37);
	if (col == 2.0) return RGB(227,157,37);
	if (col == 3.0) return RGB(106,107,4);
	return RGB(0,0,0);
}

vec4 sprMarioWalk3(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,0.,0.,0.,0.,0.,1.,1.,1., 1.,1.,0.,0.,0.,0.,0.,0.);
	if (y == 14) col = SPRROW(x,0.,0.,0.,0.,1.,1.,1.,1., 1.,1.,1.,1.,1.,0.,0.,0.);
	if (y == 13) col = SPRROW(x,0.,0.,0.,0.,3.,3.,3.,2., 2.,3.,2.,0.,0.,0.,0.,0.);
	if (y == 12) col = SPRROW(x,0.,0.,0.,3.,2.,3.,2.,2., 2.,3.,2.,2.,2.,0.,0.,0.);
	if (y == 11) col = SPRROW(x,0.,0.,0.,3.,2.,3.,3.,2., 2.,2.,3.,2.,2.,2.,0.,0.);
	if (y == 10) col = SPRROW(x,0.,0.,0.,3.,3.,2.,2.,2., 2.,3.,3.,3.,3.,0.,0.,0.);
	if (y ==  9) col = SPRROW(x,0.,0.,0.,0.,0.,2.,2.,2., 2.,2.,2.,2.,0.,0.,0.,0.);
	if (y ==  8) col = SPRROW(x,0.,0.,3.,3.,3.,3.,1.,1., 3.,3.,0.,0.,0.,0.,0.,0.);
	
	if (y ==  7) col = SPRROW(x,2.,2.,3.,3.,3.,3.,1.,1., 1.,3.,3.,3.,2.,2.,2.,0.);
	if (y ==  6) col = SPRROW(x,2.,2.,2.,0.,3.,3.,1.,2., 1.,1.,1.,3.,3.,2.,2.,0.);
	if (y ==  5) col = SPRROW(x,2.,2.,0.,0.,1.,1.,1.,1., 1.,1.,1.,0.,0.,3.,0.,0.);
	if (y ==  4) col = SPRROW(x,0.,0.,0.,1.,1.,1.,1.,1., 1.,1.,1.,1.,3.,3.,0.,0.);
	if (y ==  3) col = SPRROW(x,0.,0.,1.,1.,1.,1.,1.,1., 1.,1.,1.,1.,3.,3.,0.,0.);
	if (y ==  2) col = SPRROW(x,0.,3.,3.,1.,1.,1.,0.,0., 0.,1.,1.,1.,3.,3.,0.,0.);
	if (y ==  1) col = SPRROW(x,0.,3.,3.,3.,0.,0.,0.,0., 0.,0.,0.,0.,0.,0.,0.,0.);
	if (y ==  0) col = SPRROW(x,0.,0.,3.,3.,3.,0.,0.,0., 0.,0.,0.,0.,0.,0.,0.,0.);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(0,0,0);
	if (col == 1.0) return RGB(177,52,37);
	if (col == 2.0) return RGB(227,157,37);
	if (col == 3.0) return RGB(106,107,4);
	return RGB(0,0,0);
}


vec4 sprMarioWalk2(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,0.,0.,0.,0.,0.,1.,1.,1., 1.,1.,0.,0.,0.,0.,0.,0.);
	if (y == 14) col = SPRROW(x,0.,0.,0.,0.,1.,1.,1.,1., 1.,1.,1.,1.,1.,0.,0.,0.);
	if (y == 13) col = SPRROW(x,0.,0.,0.,0.,3.,3.,3.,2., 2.,3.,2.,0.,0.,0.,0.,0.);
	if (y == 12) col = SPRROW(x,0.,0.,0.,3.,2.,3.,2.,2., 2.,3.,2.,2.,2.,0.,0.,0.);
	if (y == 11) col = SPRROW(x,0.,0.,0.,3.,2.,3.,3.,2., 2.,2.,3.,2.,2.,2.,0.,0.);
	if (y == 10) col = SPRROW(x,0.,0.,0.,3.,3.,2.,2.,2., 2.,3.,3.,3.,3.,0.,0.,0.);
	if (y ==  9) col = SPRROW(x,0.,0.,0.,0.,0.,2.,2.,2., 2.,2.,2.,2.,0.,0.,0.,0.);
	if (y ==  8) col = SPRROW(x,0.,0.,0.,0.,3.,3.,1.,3., 3.,3.,0.,0.,0.,0.,0.,0.);
	
	if (y ==  7) col = SPRROW(x,0.,0.,0.,3.,3.,3.,3.,1., 1.,3.,3.,0.,0.,0.,0.,0.);
	if (y ==  6) col = SPRROW(x,0.,0.,0.,3.,3.,3.,1.,1., 2.,1.,1.,2.,0.,0.,0.,0.);
	if (y ==  5) col = SPRROW(x,0.,0.,0.,3.,3.,3.,3.,1., 1.,1.,1.,1.,0.,0.,0.,0.);
	if (y ==  4) col = SPRROW(x,0.,0.,0.,1.,3.,3.,2.,2., 2.,1.,1.,1.,0.,0.,0.,0.);
	if (y ==  3) col = SPRROW(x,0.,0.,0.,0.,1.,3.,2.,2., 1.,1.,1.,0.,0.,0.,0.,0.);
	if (y ==  2) col = SPRROW(x,0.,0.,0.,0.,0.,1.,1.,1., 3.,3.,3.,0.,0.,0.,0.,0.);
	if (y ==  1) col = SPRROW(x,0.,0.,0.,0.,0.,3.,3.,3., 3.,3.,3.,3.,0.,0.,0.,0.);
	if (y ==  0) col = SPRROW(x,0.,0.,0.,0.,0.,3.,3.,3., 3.,0.,0.,0.,0.,0.,0.,0.);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(0,0,0);
	if (col == 1.0) return RGB(177,52,37);
	if (col == 2.0) return RGB(227,157,37);
	if (col == 3.0) return RGB(106,107,4);
	return RGB(0,0,0);
}


vec4 sprMarioWalk1(int x, int y)
{
	float col = 0.0;
	if (y == 15) col = SPRROW(x,0.,0.,0.,0.,0.,0.,0.,0., 0.,0.,0.,0.,0.,0.,0.,0.);
	if (y == 14) col = SPRROW(x,0.,0.,0.,0.,0.,0.,1.,1., 1.,1.,1.,0.,0.,0.,0.,0.);
	if (y == 13) col = SPRROW(x,0.,0.,0.,0.,0.,1.,1.,1., 1.,1.,1.,1.,1.,1.,0.,0.);
	if (y == 12) col = SPRROW(x,0.,0.,0.,0.,0.,3.,3.,3., 2.,2.,3.,2.,0.,0.,0.,0.);
	if (y == 11) col = SPRROW(x,0.,0.,0.,0.,3.,2.,3.,2., 2.,2.,3.,2.,2.,2.,0.,0.);
	if (y == 10) col = SPRROW(x,0.,0.,0.,0.,3.,2.,3.,3., 2.,2.,2.,3.,2.,2.,2.,0.);
	if (y ==  9) col = SPRROW(x,0.,0.,0.,0.,3.,3.,2.,2., 2.,2.,3.,3.,3.,3.,0.,0.);
	if (y ==  8) col = SPRROW(x,0.,0.,0.,0.,0.,0.,2.,2., 2.,2.,2.,2.,2.,0.,0.,0.);
	
	if (y ==  7) col = SPRROW(x,0.,0.,0.,0.,0.,3.,3.,3., 3.,1.,3.,0.,2.,0.,0.,0.);
	if (y ==  6) col = SPRROW(x,0.,0.,0.,0.,2.,3.,3.,3., 3.,3.,3.,2.,2.,2.,0.,0.);
	if (y ==  5) col = SPRROW(x,0.,0.,0.,2.,2.,1.,3.,3., 3.,3.,3.,2.,2.,0.,0.,0.);
	if (y ==  4) col = SPRROW(x,0.,0.,0.,3.,3.,1.,1.,1., 1.,1.,1.,1.,0.,0.,0.,0.);
	if (y ==  3) col = SPRROW(x,0.,0.,0.,3.,1.,1.,1.,1., 1.,1.,1.,1.,0.,0.,0.,0.);
	if (y ==  2) col = SPRROW(x,0.,0.,3.,3.,1.,1.,1.,0., 1.,1.,1.,0.,0.,0.,0.,0.);
	if (y ==  1) col = SPRROW(x,0.,0.,3.,0.,0.,0.,0.,3., 3.,3.,0.,0.,0.,0.,0.,0.);
	if (y ==  0) col = SPRROW(x,0.,0.,0.,0.,0.,0.,0.,3., 3.,3.,3.,0.,0.,0.,0.,0.);
	
	col = SELECT(mod(float(x),8.0),col);
	if (col == 0.0) return RGB(0,0,0);
	if (col == 1.0) return RGB(177,52,37);
	if (col == 2.0) return RGB(227,157,37);
	if (col == 3.0) return RGB(106,107,4);
	return RGB(0,0,0);
}

vec4 getTile(int t, int x, int y)
{
	if (t == 0) return RGB(107,140,255);
	if (t == 1) return sprGround(x,y);
	if (t == 2) return sprQuestionBlock(x,y);
	if (t == 3) return sprUsedBlock(x,y);
	
	return RGB(107,140,255);
}

int getSection(int s, int x, int y)
{
	float col = 0.0;
	if (s == 0) {
		if (y == 6) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 5) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 4) col = SECROW(x,0.,0.,3.,3.,3.,0.,0.,0.);
		if (y == 3) col = SECROW(x,0.,0.,2.,2.,2.,0.,0.,0.);
		if (y == 2) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 1) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y <= 0) col = SECROW(x,1.,1.,1.,1.,1.,1.,1.,1.);
	}
	if (s == 1) {
		if (y == 6) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 5) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 4) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 3) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 2) col = SECROW(x,0.,0.,0.,0.,0.,1.,0.,0.);
		if (y == 1) col = SECROW(x,0.,0.,0.,1.,1.,1.,0.,0.);
		if (y <= 0) col = SECROW(x,1.,1.,1.,1.,1.,1.,1.,1.);
	}
	if (s == 2) {
		if (y == 6) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 5) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 4) col = SECROW(x,0.,0.,3.,0.,0.,3.,0.,0.);
		if (y == 3) col = SECROW(x,0.,0.,2.,0.,0.,2.,0.,0.);
		if (y == 2) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 1) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y <= 0) col = SECROW(x,1.,1.,1.,1.,1.,1.,1.,1.);
	}
	if (s == 3) {
		if (y == 6) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 5) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 4) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 3) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 2) col = SECROW(x,0.,0.,0.,1.,1.,0.,0.,0.);
		if (y == 1) col = SECROW(x,0.,0.,0.,1.,1.,1.,0.,0.);
		if (y <= 0) col = SECROW(x,1.,1.,1.,1.,1.,1.,1.,1.);
	}
	if (s == 4) {
		if (y == 6) col = SECROW(x,0.,0.,0.,0.,3.,0.,0.,0.);
		if (y == 5) col = SECROW(x,0.,0.,0.,0.,2.,0.,0.,0.);
		if (y == 4) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 3) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 2) col = SECROW(x,0.,0.,0.,1.,1.,1.,0.,0.);
		if (y == 1) col = SECROW(x,0.,0.,0.,1.,1.,1.,0.,0.);
		if (y <= 0) col = SECROW(x,1.,1.,1.,1.,1.,1.,1.,1.);
	}
	if (s == 5) {
		if (y == 6) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 5) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 4) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 3) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 2) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y == 1) col = SECROW(x,0.,0.,0.,0.,0.,0.,0.,0.);
		if (y <= 0) col = SECROW(x,1.,1.,1.,0.,0.,1.,1.,1.);
	}
	
	
	
	return int(SELECTSEC(mod(float(x),4.0),col));
}

int getBlock(int x, int y)
{
#ifdef TOTALLY_RANDOM_LEVEL
	int height = 1 + int(rand(vec2(int(float(x) / 3.0),2.3)) * 3.0);
	return (y < height ? 1 : 0);
#else
	if (y > 6) return 0;
	
	int section = int(rand(vec2(int(float(x) / 8.0),3.0)) * 6.0);
	int sectionX = int(mod(float(x), 8.0));
	
	return getSection(section,sectionX,y - int(rand(vec2(section,2.0)) * 0.0));
#endif
}

bool isSolid(int b)
{
	return (b != 0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	const float gameSpeed = 60.0;
	
	// Get the current game pixel
	// (Each game pixel is two screen pixels)
	//  (or four, if the screen is larger)
	float x = fragCoord.x / 2.0;
	float y = fragCoord.y / 2.0;
	if (iResolution.y >= 640.0) {
		x /= 2.0;
		y /= 2.0;
	}
	if (iResolution.y < 200.0) {
		x *= 2.0;
		y *= 2.0;
	}
	
	// Just move the screen up for half a block's size
	y -= 8.0;

	// Get the grid index of the block at this pixel,
	// and of the block at the screen's leftmost position
	int firstBlockX = int((iGlobalTime * gameSpeed) / 16.0);
	int blockX = int((x + iGlobalTime * gameSpeed) / 16.0);
	int blockY = int(y / 16.0);
	
	// Ask for the block ID that exists in the current position
	int block = getBlock(blockX,blockY);
	
	// Get the fractional position inside current block
	int subx = int(mod((x + iGlobalTime * gameSpeed),16.0));
	int suby = int(mod(y,16.0));
	
	// Animate block if it's a Question Block
	if (block == 2) {
		if (blockX - firstBlockX == 5) {
			suby -= int(max(0.0,(sin(mod((iGlobalTime * gameSpeed / 16.0),1.0) * 3.141592 * 1.5) * 8.0)));
		}
		
		if ((floor((x + iGlobalTime * gameSpeed) / 16.0) - (iGlobalTime * gameSpeed) / 16.0) < 4.25) block = 3;
	// Animate block if it's on top of a Question Block
	} else if (block == 3) {
		block = 2;
		suby += 16;
		if (blockX - firstBlockX == 5) {
			suby -= int(max(0.0,(sin(mod((iGlobalTime * gameSpeed / 16.0),1.0) * 3.141592 * 1.5) * 8.0)));
		}
	}
	// Get the final color for this pixel
	// (Mario can override this later on)
	fragColor = getTile(block,subx,suby);
	
	
	// If this is the column where Mario stops simulating...
	// (it's the only column he can appear in)
	if (x >= endX && x < endX + 16.0) {
		
		// Screen position in pixels:
		// Every block is 16 pixels wide
		float screenX = iGlobalTime * gameSpeed;
		
		// Mario's starting position and speed
		float marioX = screenX + startX;
		float marioY = 16.0;
		float marioXSpd = 4.0;
		float marioYSpd = 0.0;
		
		// Find out the first empty block in this column,
		// starting from the bottom, as to put Mario on top of it
		for(int i = 1; i < 4; i++) {
			if (!isSolid(getBlock(int(marioX / 16.0), i))) {
				marioY = float(i) * 16.0;
				break;
			}
		}
		
		// Number of steps to simulate;
		// We'll simulate at 15 FPS and interpolate later,
		// hence the division by 4.0
		// (Mario should actually be walking 1 pixel every 1/60th of a second,
		//  but he'll be walking 4 pixels every 1/15th)
		const int simSteps = int((endX - startX) / 4.0);
		
		// Previous position, as to interpolate later, for high frame rates
		float lastX = 0.0;
		float lastY = 0.0;
		
		// Start simulating
		bool onGround = false;
		for(int sim = 0; sim < simSteps; sim++) {
			// Store the previous position
			lastX = marioX;
			lastY = marioY;
			
			// If Mario is inside a block, move him up
			// (This happens only at the start of the simulation,
			//  sometimes because he is heads-up with a wall and
			//  cannot make a jump properly)
			onGround = false;
			if (isSolid(getBlock(int(marioX / 16.0) + 1, int(marioY / 16.0)))) {
				marioY = (floor(marioY / 16.0) * 16.0) + 16.0;
			}
			
			// Next, pretty standard platforming code
			
			// Apply gravity and move in the Y-axis
			marioYSpd -= 2.5;
			marioY += marioYSpd;
			
			// If he is going up,
			// and if there is a block above him,
			// align him with the grid (as to avoid getting inside the block),
			// and invert his YSpeed, as to fall quickly (because he bounced his head)
			if (marioYSpd > 0.0) {
				if (isSolid(getBlock(int(floor((marioX + 12.0) / 16.0)), int(floor((marioY + 15.9) / 16.0))))) {
					marioYSpd *= -0.5;
					marioY = (floor(marioY / 16.0) * 16.0);
				}
			}
			
			// If he is going down,
			// and if there is a block beneath him,
			// align him with the grid (as to land properly on top of the block),
			// and mark him as onGround (to be able to perform a jump)
			if (marioYSpd < 0.0) {
				if (isSolid(getBlock(int(floor((marioX) / 16.0)), int(floor(marioY / 16.0)))) ||
					isSolid(getBlock(int(floor((marioX + 15.9) / 16.0)), int(floor(marioY / 16.0))))) {
					marioYSpd = 0.0;
					marioY = (floor(marioY / 16.0) * 16.0) + 16.0;
					onGround = true;
				}
			}
			
			// Finally, move him in the X-axis
			// I assume here he'll never hit a block horizontally
			marioX += marioXSpd;
			
			// Now, if he's onGround,
			// and if there are blocks in front of him,
			// or if there is a pit right next to him,
			// set his YSpeed to jump
			if (onGround) {
				if (!isSolid(getBlock(int((marioX) / 16.0) + 1,0))) {
					marioYSpd = 15.5;
				} else if (isSolid(getBlock(int((marioX + 36.0) / 16.0), int((marioY + 24.0) / 16.0)))) {
					marioYSpd = 15.5;
				} else if (isSolid(getBlock(int((marioX) / 16.0) + 2, int((marioY + 8.0) / 16.0)))) {
					marioYSpd = 12.5;
				} else if (getBlock(int((marioX) / 16.0) + 1, int((marioY + 8.0) / 16.0) + 2) == 2) {
					marioYSpd = 15.5;
				}
				
			}
		}
		
		// Interpolate Y-pos for smooth high-frame-rate movement
		marioY = mix(lastY,marioY,mod(iGlobalTime * 15.0,1.0)) - 1.0;
		
		// Finally, if he appears at this row, fetch a pixel from his sprites
		if (y >= marioY && y < marioY + 16.0) {
			vec4 spr = vec4(0,0,0,0);
			if (onGround) {
				// Which frame?
				int f = int(mod(iGlobalTime * 10.0, 3.0));
				if (f == 0) spr = sprMarioWalk1(int(x - (marioX - screenX)),int(y - marioY));
				if (f == 1) spr = sprMarioWalk2(int(x - (marioX - screenX)),int(y - marioY));
				if (f == 2) spr = sprMarioWalk3(int(x - (marioX - screenX)),int(y - marioY));
			} else {
				spr = sprMarioJump(int(x - (marioX - screenX)),int(y - marioY));
			}
			// Transparency check
			if (spr.x != 0.0) fragColor = spr;
		}
	}
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
  FragCoord.y = -FragCoord.y;
  mainImage(FragColor,FragCoord);
}
