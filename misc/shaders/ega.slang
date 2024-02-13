#version 450

layout(push_constant) uniform Push
{
  vec4 SourceSize;
  vec4 OriginalSize;
  vec4 OutputSize;
  uint FrameCount;
  float palette, color_enhance;
} params;

#pragma parameter palette "Choose palette Hercules/CGA/EGA" 2.0 0.0 2.0 1.0
#define palette params.palette

#pragma parameter color_enhance "Color Enchance" 1.4 1.0 4.0 0.05
#define color_enhance params.color_enhance

#define _ 0.0
#define o (1./3.)
#define b (2./3.)
#define B 1.0


#define SourceSize params.SourceSize

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
   vTexCoord = TexCoord*1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


// Color lookup table (RGBI palette with brown fix)
const vec3 rgbi_palette[19] = 
{
  vec3(_, _, _),   // cga
  vec3(B, B, B),   // cga white
  vec3(o, B, B),   // cga cyan
  vec3(B, o, B),   // cga pink
  vec3(_, _, b),
  vec3(_, b, _),
  vec3(_, b, _),
  vec3(_, b, _),
  vec3(_, b, _),
  vec3(_, b, b),
  vec3(b, _, _),
  vec3(b, _, b),
  vec3(b, o, _),
  vec3(b, b, b),
  vec3(o, o, o),
  vec3(o, o, B),
  vec3(o, B, o),
  vec3(B, o, o),
  vec3(B, B, o),
 
};

const vec3 cga_palette[4] = 
{
  vec3(_, _, _),   // cga
  vec3(o, B, B),   // cga cyan
  vec3(B, o, B),   // cga pink
  vec3(B, B, B),   // cga white
};

const vec3 simcga_palette[2] = 
{
  vec3(_, _, _),   // black
  vec3(o, b, o),   // green

};


// Came off the wikipedia(R) article for Ordered_Dithering
const int dithertable[16] = 
{
	 1, 9, 3,11,
	13, 5,15, 7,
	 4,12, 2,10,
  16, 8,14, 6,
};


// Compare vector distances and return nearest RGBI color
vec3 nearest_rgbi (vec3 original) 
{
  float dst;
  float min_dst = 2.0;
  int idx = 0;

  int set; if (palette == 2.0)set = 19; else if (palette == 1.0) set = 4;else if (palette == 1.0) set = 2;
  for (int i=0; i<set; i++) 
  {
    dst = distance(original, rgbi_palette[i]);
    if (dst < min_dst) 
     {
      min_dst = dst;
      idx = i;
     }
  }
  if (palette == 2.0) return rgbi_palette[idx];
  else if (palette == 1.0) return cga_palette[idx];
  else if (palette == 0.0) return simcga_palette[idx];
 
}


float modu(float x, float y)
{
	return x - y * floor(x/y);
}


void main()
{
  vec3 color = texture(Source, vTexCoord.xy).rgb;

	// leilei - dither 
	vec2 res;
	vec2 tex=vTexCoord.xy;
	res.x = SourceSize.x; 

	if (SourceSize.y < 900.0)
		res.y = SourceSize.y; 
	   else
	     res.y = SourceSize.y / 2.0; 

	int ohyes;
  int i;
  vec2 ditheu = tex.xy * res.xy;

  int ditdex =  int(modu(ditheu.x, 4))*4 + int(modu(ditheu.y, 4)); // 4x4!

  // looping through a lookup table matrix
  
  for (i = ditdex; i < (ditdex+16); i = i+1) 
      ohyes = dithertable[i-15] * 2 - 21;
      ohyes = ohyes - 6;
      
      color *= 255.0;
      color += ohyes;
      color /= 255.0;
     

      vec3 final = nearest_rgbi(color*color_enhance);
 

  FragColor = vec4(final, 1.0);
}
