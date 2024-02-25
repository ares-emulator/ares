#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float postfilter_taps, postfilter_chromacutoff;
} params;

  // after mixing with the adaptive comb filter
#pragma parameter postfilter_taps "Post-filter Taps" 15.0 1.0 30.0 1.0
#define postfiltertaps int(params.postfilter_taps)
#pragma parameter postfilter_chromacutoff "Post-filter Chroma Cutoff" 0.3 0.1 1.0 0.05
#define postfilterchromacutoff params.postfilter_chromacutoff

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 texCoord;

void main()
{
   gl_Position = global.MVP * Position;
   texCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec3 chroma_sample(vec2 shift) {
  vec2 coord = texCoord + params.SourceSize.zw*shift;
  if (coord.x < 0.0 || coord.x > 1.0) return vec3(0.0);
  return texture(Source, texCoord + params.SourceSize.zw*shift).xyz-vec3(0,0.5,0.5);
}

float sinc(float x) {
  x = max(abs(x),0.0001);
  return sin(x)/x;
}

void main()
{
  const int width = postfiltertaps;
  const float bw_c = postfilterchromacutoff/3.0;

  const vec4 one = vec4(1.0);
  const float PI = 3.14159265359;
  const vec4 PI_6 = vec4(PI/6.0);

  float norm_c = 0.0;
  vec3 yiq = vec3(0.0);
  yiq.x = chroma_sample(vec2(0,0)).x;
  for (int i = -width/2; i <= width/2; i++) {
    // Hamming window
    const float alpha = 0.54;
    const float beta  = 0.46;
    float window = alpha - beta * cos(2.0*PI/(width-1)*(width/2+i));

    float chromafilt = 2.0*bw_c*sinc(2.0*PI*bw_c*i);

    vec3 samp = chroma_sample(vec2(i,0));
    float filt = window*chromafilt;
    yiq.yz += samp.yz*vec2(filt);
    norm_c += filt;
  }

  yiq *= vec3(1.0, 1.0/norm_c, 1.0/norm_c);

  FragColor = vec4(dot(yiq, vec3(1.0, 0.946882, 0.623557)),
		   dot(yiq, vec3(1.0,-0.274788,-0.635691)),
		   dot(yiq, vec3(1.0,-1.108545, 1.709007)),
		   0.0);
}
