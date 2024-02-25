#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float minimum;
   float maximum;
} params;

#pragma parameter minimum "Edge Thresh Min" 0.05 0.0 1.0 0.01
#pragma parameter maximum "Edge Thresh Max" 0.35 0.0 1.0 0.01

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

float threshold(float thr1, float thr2 , float val) {
 val = (val < thr1) ? 0.0 : val;
 val = (val > thr2) ? 1.0 : val;
 return val;
}

// averaged pixel intensity from 3 color channels
float avg_intensity(vec4 pix) {
 return dot(pix.rgb, vec3(0.2126, 0.7152, 0.0722));
}

vec4 get_pixel(sampler2D tex, vec2 coords, float dx, float dy) {
 return texture(tex, coords + vec2(dx, dy));
}

// returns pixel color
float IsEdge(sampler2D tex, vec2 coords){
  float dxtex = params.SourceSize.z;
  float dytex = params.SourceSize.w;
  float pix[9];
  int k = -1;
  float delta;

  // read neighboring pixel intensities
  for (int i=-1; i<2; i++) {
   for(int j=-1; j<2; j++) {
    k++;
    pix[k] = avg_intensity(get_pixel(tex, coords, float(i) * dxtex,
                                          float(j) * dytex));
   }
  }

  // average color differences around neighboring pixels
  delta = (abs(pix[1]-pix[7])+
          abs(pix[5]-pix[3]) +
          abs(pix[0]-pix[8])+
          abs(pix[2]-pix[6])
           )/4.;

  return threshold(params.minimum, params.maximum,clamp(delta,0.0,1.0));
}

void main()
{
   float test = IsEdge(Source, vTexCoord);
   FragColor = vec4(test);
}
