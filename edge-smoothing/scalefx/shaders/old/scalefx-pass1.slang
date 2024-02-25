#version 450

/*
	ScaleFX - Pass 1
	by Sp00kyFox, 2016-03-30

Filter:	Nearest
Scale:	1x

ScaleFX is an edge interpolation algorithm specialized in pixel art. It was
originally intended as an improvement upon Scale3x but became a new filter in
its own right.
ScaleFX interpolates edges up to level 6 and makes smooth transitions between
different slopes. The filtered picture will only consist of colours present
in the original.

Pass 1 resolves ambiguous configurations of corner candidates at pixel junctions.



Copyright (c) 2016 Sp00kyFox - ScaleFX@web.de

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#pragma name sfxp1
#pragma parameter SFX_CLR "ScaleFX Color Thresh" 0.35 0.0 1.00 0.01

#ifdef PARAMETER_UNIFORM
	uniform float SFX_CLR;
#else
	#define SFX_CLR   0.35
#endif

const float THR = 1.0 - SFX_CLR;


layout(set = 0, binding = 0, std140) uniform UBO
{
   mat4 MVP;
   vec4 SourceSize;
};


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = MVP * Position;
	vTexCoord = TexCoord;
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 1) uniform sampler2D Source;


#define LE(x, y) (1.0 - step(y, x))
#define GE(x, y) (1.0 - step(x, y))
#define LEQ(x, y) step(x, y)
#define GEQ(x, y) step(y, x)
#define NOT(x) (1.0 - (x))


// corner strength
vec4 str(vec4 crn, vec4 ort){
	//return (crn > THR) ? max(2.0*crn - (ort + ort.wxyz), 0.0) : 0.0;
	return GE(crn, vec4(THR)) * max(2.0*crn - (ort + ort.wxyz), vec4(0.0));
}

// corner dominance at junctions
vec4 dom(vec3 strx, vec3 stry, vec3 strz, vec3 strw){
	vec4 res;
	res.x = max(2.0*strx.y - (strx.x + strx.z), 0.0);
	res.y = max(2.0*stry.y - (stry.x + stry.z), 0.0);
	res.z = max(2.0*strz.y - (strz.x + strz.z), 0.0);
	res.w = max(2.0*strw.y - (strw.x + strw.z), 0.0);
	return res;
}

// necessary but not sufficient junction condition for orthogonal edges
float clear(vec2 crn, vec4 ort){
	//return all(crn.xyxy <= THR || crn.xyxy <= ort || crn.xyxy <= ort.wxyz);
	vec4 res = LEQ(crn.xyxy, vec4(THR)) + LEQ(crn.xyxy, ort) + LEQ(crn.xyxy, ort.wxyz);
	return min(res.x * res.y * res.z * res.w, 1.0);
}


void main()
{

	/*	grid		metric		pattern

		M A B C P	x y z		x y
		N D E F Q	  o w		w z
		O G H I R
		  J K L
	*/


#define TEX(x, y) textureOffset(Source, vTexCoord, ivec2(x, y))


	// metric data
	vec4 A = TEX(-1,-1), B = TEX( 0,-1), C = TEX( 1,-1);
	vec4 D = TEX(-1, 0), E = TEX( 0, 0), F = TEX( 1, 0);
	vec4 G = TEX(-1, 1), H = TEX( 0, 1), I = TEX( 1, 1);
	vec4 J = TEX(-1, 2), K = TEX( 0, 2), L = TEX( 1, 2);
	vec4 M = TEX(-2,-1), N = TEX(-2, 0), O = TEX(-2, 1);
	vec4 P = TEX( 2,-1), Q = TEX( 2, 0), R = TEX( 2, 1);


	// corner strength
	vec4 As = str(vec4(M.z, B.x, D.zx), vec4(A.yw, D.y, M.w));
	vec4 Bs = str(vec4(A.z, C.x, E.zx), vec4(B.yw, E.y, A.w));
	vec4 Cs = str(vec4(B.z, P.x, F.zx), vec4(C.yw, F.y, B.w));
	vec4 Ds = str(vec4(N.z, E.x, G.zx), vec4(D.yw, G.y, N.w));
	vec4 Es = str(vec4(D.z, F.x, H.zx), vec4(E.yw, H.y, D.w));
	vec4 Fs = str(vec4(E.z, Q.x, I.zx), vec4(F.yw, I.y, E.w));
	vec4 Gs = str(vec4(O.z, H.x, J.zx), vec4(G.yw, J.y, O.w));
	vec4 Hs = str(vec4(G.z, I.x, K.zx), vec4(H.yw, K.y, G.w));
	vec4 Is = str(vec4(H.z, R.x, L.zx), vec4(I.yw, L.y, H.w));

	// strength & dominance junctions
	vec4 jSx = vec4(As.z, Bs.w, Es.x, Ds.y), jDx = dom(As.yzw, Bs.zwx, Es.wxy, Ds.xyz);
	vec4 jSy = vec4(Bs.z, Cs.w, Fs.x, Es.y), jDy = dom(Bs.yzw, Cs.zwx, Fs.wxy, Es.xyz);
	vec4 jSz = vec4(Es.z, Fs.w, Is.x, Hs.y), jDz = dom(Es.yzw, Fs.zwx, Is.wxy, Hs.xyz);
	vec4 jSw = vec4(Ds.z, Es.w, Hs.x, Gs.y), jDw = dom(Ds.yzw, Es.zwx, Hs.wxy, Gs.xyz);


	// majority vote for ambiguous dominance junctions
	//bvec4 jx = jDx != 0.0 && jDx + jDx.zwxy > jDx.yzwx + jDx.wxyz;
	//bvec4 jy = jDy != 0.0 && jDy + jDy.zwxy > jDy.yzwx + jDy.wxyz;
	//bvec4 jz = jDz != 0.0 && jDz + jDz.zwxy > jDz.yzwx + jDz.wxyz;
	//bvec4 jw = jDw != 0.0 && jDw + jDw.zwxy > jDw.yzwx + jDw.wxyz;

	vec4 jx = GE(jDx, vec4(0.0)) * GE(jDx + jDx.zwxy, jDx.yzwx + jDx.wxyz);
	vec4 jy = GE(jDy, vec4(0.0)) * GE(jDy + jDy.zwxy, jDy.yzwx + jDy.wxyz);
	vec4 jz = GE(jDz, vec4(0.0)) * GE(jDz + jDz.zwxy, jDz.yzwx + jDz.wxyz);
	vec4 jw = GE(jDw, vec4(0.0)) * GE(jDw + jDw.zwxy, jDw.yzwx + jDw.wxyz);

	// inject strength without creating new contradictions
	//bvec4 res;
	//res.x = jx.z || !(jx.y || jx.w) && (jSx.z != 0.0 && (jx.x || jSx.x + jSx.z > jSx.y + jSx.w));
	//res.y = jy.w || !(jy.z || jy.x) && (jSy.w != 0.0 && (jy.y || jSy.y + jSy.w > jSy.x + jSy.z));
	//res.z = jz.x || !(jz.w || jz.y) && (jSz.x != 0.0 && (jz.z || jSz.x + jSz.z > jSz.y + jSz.w));
	//res.w = jw.y || !(jw.x || jw.z) && (jSw.y != 0.0 && (jw.w || jSw.y + jSw.w > jSw.x + jSw.z));

	vec4 res;
	res.x = min(jx.z + (NOT(jx.y) * NOT(jx.w)) * (GE(jSx.z, 0.0) * (jx.x + GE(jSx.x + jSx.z, jSx.y + jSx.w))), 1.0);
	res.y = min(jy.w + (NOT(jy.z) * NOT(jy.x)) * (GE(jSy.w, 0.0) * (jy.y + GE(jSy.y + jSy.w, jSy.x + jSy.z))), 1.0);
	res.z = min(jz.x + (NOT(jz.w) * NOT(jz.y)) * (GE(jSz.x, 0.0) * (jz.z + GE(jSz.x + jSz.z, jSz.y + jSz.w))), 1.0);
	res.w = min(jw.y + (NOT(jw.x) * NOT(jw.z)) * (GE(jSw.y, 0.0) * (jw.w + GE(jSw.y + jSw.w, jSw.x + jSw.z))), 1.0);	


	// single pixel & end of line detection
	//res = res && (bvec4(jx.z, jy.w, jz.x, jw.y) || !(res.wxyz && res.yzwx));	
	res = min(res * (vec4(jx.z, jy.w, jz.x, jw.y) + NOT(res.wxyz * res.yzwx)), vec4(1.0));


	// output

	vec4 clr;
	clr.x = clear(vec2(D.z, E.x), vec4(A.w, E.y, D.wy));
	clr.y = clear(vec2(E.z, F.x), vec4(B.w, F.y, E.wy));
	clr.z = clear(vec2(H.z, I.x), vec4(E.w, I.y, H.wy));
	clr.w = clear(vec2(G.z, H.x), vec4(D.w, H.y, G.wy));

	vec4 low = max(vec4(E.yw, H.y, D.w), vec4(THR));
	
	vec4 hori = vec4(low.x < max(D.w, A.w), low.x < max(E.w, B.w), low.z < max(E.w, H.w), low.z < max(D.w, G.w)) * clr;	// horizontal edges
	vec4 vert = vec4(low.w < max(E.y, D.y), low.y < max(E.y, F.y), low.y < max(H.y, I.y), low.w < max(H.y, G.y)) * clr;	// vertical edges
	vec4 orie  = vec4(A.w < D.y, B.w <= F.y, H.w < I.y, G.w <= G.y);								// orientation

	FragColor = (res + 2.0 * hori + 4.0 * vert + 8.0 * orie) / 15.0;
}
