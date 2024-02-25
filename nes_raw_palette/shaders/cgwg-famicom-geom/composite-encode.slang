#version 450
#pragma name encode_pass

layout(push_constant) uniform Push
{
   vec4 SourceSize;
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

bool wave (int p, int color)
{
   return ((color + p + 8) % 12 < 6);
}

void main()
{
/* original palette decode for historical purposes:
  uint n = uint(texture(Source, vTexCoord).r);
  uint color = n & 0xfu;
  uint level = color < 0xeu ? (n>>4u)&3u : 1u;
uint emphasis = n >> 6u;
*/

   vec4 c = texture(Source, vTexCoord.xy);

   // Extract the chroma, level, and emphasis from the normalized RGB triplet
   int color =    int(floor((c.r * 15.0) + 0.5));
   int level =    int(floor((c.g *  3.0) + 0.5));
   int emphasis = int(floor((c.b *  7.0) + 0.5));

  const float levels[] = float[](0.350, 0.518, 0.962, 1.550,
				 1.094, 1.506, 1.962, 1.962);
  const float attenuation = 0.746;
  const float min = levels[0]*attenuation;
  const float max = levels[7];

  float lo_hi[] = float[](levels[level + 4u*uint(color == 0x0u)],
			  levels[level + 4u*uint(color <  0xdu)]);

  uint x = uint(floor(vTexCoord.x*params.SourceSize.x*2.0));
  uint y = uint(floor(vTexCoord.y*params.SourceSize.y));
  float frag[4];
  for (uint i = 0u; i < 4u; i++) {
    uint p = (x*4u+i + y*4u + uint(params.FrameCount*4)) % 12u;
#define wave(ph,co) (((co)+(ph)+8u)%12u<6u)
    float spot = lo_hi[uint(wave(p,color))];
    if ( (((emphasis&1u)==1u) && wave(p,12u))
	 || (((emphasis&2u)==1u) && wave(p,4u))
	 || (((emphasis&4u)==1u) && wave(p,8u)))
      spot *= attenuation;
    frag[i] = spot;
  }
  FragColor = (vec4(frag[0], frag[1], frag[2], frag[3]) - vec4(min))/vec4(max-min);
}
