#version 450

/*
	Smart Morph v1.1
	by Sp00kyFox, 2014


Determines the brightest (or darkest) pixel X in the orthogonal neighborship of pixel E (including itself).
Output is a linear combination of X and E weighted with their luma difference d (range: [0.0, 1.0]):

w = sstp{sat[(d - CUTLO)/(CUTHI - CUTLO)]^PWR} * (STRMAX - STRMIN) + STRMIN

with
sstp(x) := smoothstep(0, 1, x) = -2x^3 + 3x^2
sat(x)  := saturate(x) = max(0, min(1, x))

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float SM_MODE, SM_RANGE, SM_PWR, SM_STRMIN, SM_STRMAX, SM_CUTLO, SM_CUTHI, SM_DEBUG;
} params;

#pragma parameter SM_MODE   "SmartMorph Dilation / Erosion"	0.0 0.0 1.0 1.0		// Switches between dilation and erosion (line thinning or thickening).
#pragma parameter SM_RANGE  "SmartMorph Range 0-multi 1-hor 2-vert"	0.0 0.0 2.0 1.0	// Switches between all directions / horizontal only / vertical only.
#pragma parameter SM_PWR    "SmartMorph Luma Exponent"		0.5 0.0 10.0 0.1	// range: [0.0, +inf)   - Raises d by the exponent of PWR. Smaller values for stronger morphing.
#pragma parameter SM_STRMIN "SmartMorph MIN Strength"		0.0 0.0 1.0 0.01	// range: [0.0, STRMAX] - Minimal strength to apply
#pragma parameter SM_STRMAX "SmartMorph MAX Strength"		1.0 0.0 1.0 0.01	// range: [STRMIN, 1.0] - Maximal strength to apply.
#pragma parameter SM_CUTLO  "SmartMorph LO Contrast Cutoff"	0.0 0.0 1.0 0.01	// range: [0.0, CUTHI)  - Cutoff for low contrasts. For d smaller than CUTLO, STRMIN is applied.
#pragma parameter SM_CUTHI  "SmartMorph HI Contrast Cutoff"	1.0 0.0 1.0 0.01	// range: (CUTLO, 1.0]  - Cutoff for high contrasts. For d bigger than CUTHI, STRMAX is applied.
#pragma parameter SM_DEBUG  "SmartMorph Adjust View"		0.0 0.0 1.0 1.0

// STRMIN = CUTLO = 1.0 behaves equivalent to Hyllian's shaders.

const vec3 y_weights = vec3(0.299, 0.587, 0.114);

#define TEX(dx,dy) texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)
#define mul(c,d) (d*c)

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
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	vec3 B = TEX( 0.,-1.).rgb;
	vec3 D = TEX(-1., 0.).rgb;
	vec3 E = TEX( 0., 0.).rgb;
	vec3 F = TEX( 1., 0.).rgb;
	vec3 H = TEX( 0., 1.).rgb;
   vec3 res;

	float  e = dot(E, y_weights);
	float di = 0.0;

   if (params.SM_RANGE == 2.0){
      vec2 b = mul( mat2x3(B, H), y_weights );
      if(params.SM_MODE > 0.5)
         di = max(b.x, b.y);
      else
         di = min(b.x, b.y);

      res = (di==b.y) ? B : H;
   }
   else if (params.SM_RANGE == 1.0){
      vec2 b = mul( mat2x3(D,F), y_weights );
      if(params.SM_MODE > 0.5)
         di = max(b.x, b.y);
      else
         di = min(b.x, b.y);

      res = (di==b.y) ? D : F;
   }
   else{
      vec4 b = mul( mat4x3(B, D, H, F), y_weights );
      if(params.SM_MODE > 0.5)
         di = min(min(b.x, b.y), min(b.z, min(b.w, e)));
      else
         di = max(max(b.x, b.y), max(b.z, max(b.w, e)));

      res = (di==b.x) ? B : (di==b.y) ? D : (di==b.z) ? H : F;
   }

	di = abs(e-di);

	float str = (di<=params.SM_CUTLO) ? params.SM_STRMIN : (di>=params.SM_CUTHI) ? params.SM_STRMAX : smoothstep( 0.0, 1.0, pow((di-params.SM_CUTLO)/(params.SM_CUTHI-params.SM_CUTLO), params.SM_PWR) ) * (params.SM_STRMAX-params.SM_STRMIN) + params.SM_STRMIN;

	if(params.SM_DEBUG > 0.5){
		FragColor.rgb = vec3(str);
      return;
   }

	FragColor.rgb = (di==0.0) ? E : mix(E, res, str);
}