#version 450

// MMPX
// by Morgan McGuire and Mara Gagiu
// https://casual-effects.com/research/McGuire2021PixelArt/
// License: MIT
// adapted for slang by hunterk

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
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

float luma(vec3 col){
   return dot(col, vec3(0.2126, 0.7152, 0.0722));
}

// I tried using a hash to speed these up but it didn't really help
bool same(vec3 B, vec3 A0){
   return all(equal(B, A0));
}

bool notsame(vec3 B, vec3 A0){
   return any(notEqual(B, A0));
}

bool all_eq2(vec3 B, vec3 A0, vec3 A1) {
    return (same(B,A0) && same(B,A1));
}

bool all_eq3(vec3 B, vec3 A0, vec3 A1, vec3 A2) {
    return (same(B,A0) && same(B,A1) && same(B,A2));
}

bool all_eq4(vec3 B, vec3 A0, vec3 A1, vec3 A2, vec3 A3) {
    return (same(B,A0) && same(B,A1) && same(B,A2) && same(B,A3));
}

bool any_eq3(vec3 B, vec3 A0, vec3 A1, vec3 A2) {
   return (same(B,A0) || same(B,A1) || same(B,A2));
}

bool none_eq2(vec3 B, vec3 A0, vec3 A1) {
   return (notsame(B,A0) && notsame(B,A1));
}

bool none_eq4(vec3 B, vec3 A0, vec3 A1, vec3 A2, vec3 A3) {
   return (notsame(B,A0) && notsame(B,A1) && notsame(B,A2) && notsame(B,A3));
}

#define src(c,d) texture(Source, vTexCoord + vec2(c,d) * params.SourceSize.zw).rgb

void main()
{
// these do nothing, but just for consistency with the original code...
   float srcX = 0.;
   float srcY = 0.;

// Our current pixel
   vec3 E = src(srcX+0.,srcY+0.);

// Input: A-I central 3x3 grid
   vec3 A = src(srcX-1.,srcY-1.);
   vec3 B = src(srcX+0.,srcY-1.);
   vec3 C = src(srcX+1.,srcY-1.);

   vec3 D = src(srcX-1.,srcY+0.);
   vec3 F = src(srcX+1.,srcY+0.);

   vec3 G = src(srcX-1.,srcY+1.);
   vec3 H = src(srcX+0.,srcY+1.);
   vec3 I = src(srcX+1.,srcY+1.);

// Default to Nearest magnification
   vec3 J = E;
   vec3 K = E;
   vec3 L = E;
   vec3 M = E;
// Go ahead and put this here so we can use an early return on the next
// line to save some cycles
   FragColor = vec4(E, 1.0);
   
// Skip constant 3x3 centers and just use nearest-neighbor
// them.  This gives a good speedup on spritesheets with
// lots of padding and full screen images with large
// constant regions such as skies.
// EDIT: this is a wash for me, but we'll keep it around
 if(same(E,A) && same(E,B) && same(E,C) && same(E,D) && same(E,F) && same(E,G) && same(E,H) && same(E,I)) return;

// Read additional values at the tips of the diamond pattern
   vec3 P  = src(srcX+0.,srcY-2.);
   vec3 Q  = src(srcX-2.,srcY+0.);
   vec3 R  = src(srcX+2.,srcY+0.);
   vec3 S  = src(srcX+0.,srcY+2.);

// Precompute luminances
   float Bl = luma(B);
   float Dl = luma(D);
   float El = luma(E);
   float Fl = luma(F);
   float Hl = luma(H);

// Round some corners and fill in 1:1 slopes, but preserve
// sharp right angles.
//
// In each expression, the left clause is from
// EPX and the others are new. EPX
// recognizes 1:1 single-pixel lines because it
// applies the rounding only to the LINE, and not
// to the background (it looks at the mirrored
// side).  It thus fails on thick 1:1 edges
// because it rounds *both* sides and produces an
// aliased edge shifted by 1 dst pixel.  (This
// also yields the mushroom-shaped arrow heads,
// where that 1-pixel offset runs up against the
// 2-pixel aligned end; this is an inherent
// problem with 2X in-palette scaling.)
//
// The 2nd clause clauses avoid *double* diagonal
// filling on 1:1 slopes to prevent them becoming
// aliased again. It does this by breaking
// symmetry ties using luminance when working with
// thick features (it allows thin and transparent
// features to pass always).
//
// The 3rd clause seeks to preserve square corners
// by considering the center value before
// rounding.
//
// The 4th clause identifies 1-pixel bumps on
// straight lines that are darker than their
// background, such as the tail on a pixel art
// "4", and prevents them from being rounded. This
// corrects for asymmetry in this case that the
// luminance tie breaker introduced.

// .------------ 1st ------------.      .----- 2nd ---------.      .------ 3rd -----.      .--------------- 4th -----------------------.
   if (((same(D,B) && notsame(D,H) && notsame(D,F))) && ((El>=Dl) || same(E,A)) && any_eq3(E,A,C,G) && ((El<Dl) || notsame(A,D) || notsame(E,P) || notsame(E,Q))) J=D;
   if (((same(B,F) && notsame(B,D) && notsame(B,H))) && ((El>=Bl) || same(E,C)) && any_eq3(E,A,C,I) && ((El<Bl) || notsame(C,B) || notsame(E,P) || notsame(E,R))) K=B;
   if (((same(H,D) && notsame(H,F) && notsame(H,B))) && ((El>=Hl) || same(E,G)) && any_eq3(E,A,G,I) && ((El<Hl) || notsame(G,H) || notsame(E,S) || notsame(E,Q))) L=H;
   if (((same(F,H) && notsame(F,B) && notsame(F,D))) && ((El>=Fl) || same(E,I)) && any_eq3(E,C,G,I) && ((El<Fl) || notsame(I,H) || notsame(E,R) || notsame(E,S))) M=F;

// Clean up disconnected line intersections.
//
// The first clause recognizes being on the inside
// of a diagonal corner and ensures that the "foreground"
// has been correctly identified to avoid
// ambiguous cases such as this:
//
//  o#o#
//  oo##
//  o#o#
//
// where trying to fix the center intersection of
// either the "o" or the "#" will leave the other
// one disconnected. This occurs, for example,
// when a pixel-art letter "B" or "R" is next to
// another letter on the right.
//
// The second clause ensures that the pattern is
// not a notch at the edge of a checkerboard
// dithering pattern.
// 
// >
//  .--------------------- 1st ------------------------.      .--------- 2nd -----------. 
   if ((notsame(E,F) && all_eq4(E,C,I,D,Q) && all_eq2(F,B,H)) && notsame(F,src(srcX+3.,srcY))) K=M=F;
   if ((notsame(E,D) && all_eq4(E,A,G,F,R) && all_eq2(D,B,H)) && notsame(D,src(srcX-3.,srcY))) J=L=D;
   if ((notsame(E,H) && all_eq4(E,G,I,B,P) && all_eq2(H,D,F)) && notsame(H,src(srcX,srcY+3.))) L=M=H;
   if ((notsame(E,B) && all_eq4(E,A,C,H,S) && all_eq2(B,D,F)) && notsame(B,src(srcX,srcY-3.))) J=K=B;

// Remove tips of bright triangles on dark
// backgrounds. The luminance tie breaker for 1:1
// pixel lines leaves these as sticking up squared
// off, which makes bright triangles and diamonds
// look bad.
   if ((Bl<El) && all_eq4(E,G,H,I,S) && none_eq4(E,A,D,C,F)) J=K=B;
   if ((Hl<El) && all_eq4(E,A,B,C,P) && none_eq4(E,D,G,I,F)) L=M=H;
   if ((Fl<El) && all_eq4(E,A,D,G,Q) && none_eq4(E,B,C,I,H)) K=M=F;
   if ((Dl<El) && all_eq4(E,C,F,I,R) && none_eq4(E,B,A,G,H)) J=L=D;

//////////////////////////////////////////////////////////////////////////////////
// Do further neighborhood peeking to identify
// 2:1 and 1:2 slopes of constant color.
// The first clause of each rule identifies a 2:1 slope line
// of consistent color.
//
// The second clause verifies that the line is separated from
// every adjacent pixel on one side and not part of a more
// complex pattern. Common subexpressions from the second clause
// are lifted to an outer test on pairs of rules.
// 
// The actions taken by rules are unusual in that they extend
// a color assigned by previous rules rather than drawing from
// the original source image.
//
// The comments show a diagram of the local
// neighborhood in which letters shown with the
// same shape and color must match each other and
// everything else without annotation must be
// different from the solid colored, square
// letters.
   if (notsame(H,B)) { // Common subexpression
                       // Above a 2:1 slope or -2:1 slope   ◢ ◣
                       // First:
      if (notsame(H,A) && notsame(H,E) && notsame(H,C)) {
                       // Second:
                       //     P 
                       //   Ⓐ B C .
                       // Q D 🄴 🅵 🆁
                       //   🅶 🅷 I
                       //     S
         if (all_eq3(H,G,F,R) && none_eq2(H,D,src(srcX+2.,srcY-1.))) L=M;
                       // Third:
                       //     P 
                       // . A B Ⓒ
                       // 🆀 🅳 🄴 F R
                       //   G 🅷 🅸
                       //     S
         if (all_eq3(H,I,D,Q) && none_eq2(H,F,src(srcX-2.,srcY-1.))) M=L;
      }
                        
                       // Below a 2:1 (◤) or -2:1 (◥) slope (reflect the above 2:1 patterns vertically)
      if (notsame(B,I) && notsame(B,G) && notsame(B,E)) {
                       //     P 
                       //   🅰 🅱 C
                       // Q D 🄴 🅵 🆁
                       //   Ⓖ H I .
                       //     S
         if (all_eq3(B,A,F,R) && none_eq2(B,D,src(srcX+2.,srcY+1.))) J=K;
                       //     P 
                       //   A 🅱 🅲
                       // 🆀 🅳 🄴 F R
                       // . G H Ⓘ 
                       //     S
         if (all_eq3(B,C,D,Q) && none_eq2(B,F,src(srcX-2.,srcY+1.))) K=J;
      }
   }

   if (notsame(F,D)) { // Common subexpression
                        
                       // Right of a -1:2 (\) or -1:2 (/) slope (reflect the left 1:2 patterns horizontally)
      if (notsame(D,I) && notsame(D,E) && notsame(D,C)) {
                       //     P
                       //   🅰 B Ⓒ
                       // Q 🅳 🄴 F R
                       //   G 🅷 I
                       //     🆂 .
         if (all_eq3(D,A,H,S) && none_eq2(D,B,src(srcX+1.,srcY+2.))) J=L;
                       //     🅿 .
                       //   A 🅱 C
                       // Q 🅳 🄴 F R
                       //   🅶 H Ⓘ
                       //     S
         if (all_eq3(D,G,B,P) && none_eq2(D,H,src(srcX+1.,srcY-2.))) L=J;
      }
                        
                       // Left of a 1:2 (/) slope or -1:2 (\) slope (transpose the above 2:1 patterns)
                       // Pull common none_eq subexpressions out

      if (notsame(F,E) && notsame(F,A) && notsame(F,G)) {
                       //     P     
                       //   Ⓐ B 🅲   
                       // Q D 🄴 🅵 R 
                       //   G 🅷 I   
                       //   . 🆂   
         if (all_eq3(F,C,H,S) && none_eq2(F,B,src(srcX-1.,srcY+2.))) K=M;
                       //   . 🅿
                       //   A 🅱 C
                       // Q D 🄴 🅵 R
                       //   Ⓖ H 🅸
                       //     S
         if (all_eq3(F,I,B,P) && none_eq2(F,H,src(srcX-1.,srcY-2.))) M=K;
      }
   }

// Determine which of our 4 output pixels we need to use
   vec2 a = fract(vTexCoord * params.SourceSize.xy);
   FragColor.rgb = (a.x < 0.5) ? (a.y < 0.5 ? J : L) : (a.y < 0.5 ? K : M);
}
