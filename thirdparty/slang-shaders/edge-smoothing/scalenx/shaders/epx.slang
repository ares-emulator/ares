#version 450

// EPX (Eric's Pixel Scaler)
// based on the description from Wikipedia:
// https://en.wikipedia.org/wiki/Pixel-art_scaling_algorithms#EPX/Scale2%C3%97/AdvMAME2%C3%97
// adapted for slang by hunterk
// license GPL, I think

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

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

bool same(vec3 B, vec3 A0){
   return all(equal(B, A0));
}

bool notsame(vec3 B, vec3 A0){
   return any(notEqual(B, A0));
}

// sample with coordinate offsets
#define TEX(c,d) texture(Source, vTexCoord.xy + vec2(c,d) * params.SourceSize.zw).rgb

void main()
{
// The algorithm looks at the current pixel and the 4 surrounding cardinal pixels  
// ___|_A_|___  
// _C_|_P_|_B_
//    | D |  

// Our current pixel, P
   vec3 P = TEX( 0., 0.);

// Input pixels
   vec3 A = TEX( 0., 1.);
   vec3 B = TEX( 1., 0.);
   vec3 D = TEX( 0.,-1.);
   vec3 C = TEX(-1., 0.);
   
// Output: 2x2 grid. Default to the current pixel color (Nearest magnification)
// ___one_|_two___
//  three | four
   vec3 one   = P;
   vec3 two   = P;
   vec3 three = P;
   vec3 four  = P;

// EPX algorithm rules:   
// IF C==A AND C!=D AND A!=B => 1=A
// IF A==B AND A!=C AND B!=D => 2=B
// IF D==C AND D!=B AND C!=A => 3=C
// IF B==D AND B!=A AND D!=C => 4=D

   one   = (same(C, D) && notsame(C, A) && notsame(C, B)) ? C : P;
   two   = (same(D, B) && notsame(D, C) && notsame(D, A)) ? D : P;
   three = (same(A, C) && notsame(A, B) && notsame(A, D)) ? A : P;
   four  = (same(B, A) && notsame(B, D) && notsame(B, C)) ? B : P;

   vec2 px = fract(vTexCoord * params.SourceSize.xy);
// split the texels into 4 and assign one of our output pixels to each
   FragColor.rgb = (px.x < 0.5) ? (px.y < 0.5 ? one : three) : (px.y < 0.5 ? two : four);
   FragColor.a = 1.0;
}
