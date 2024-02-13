#version 450
// proof of concept procedural shader 
// done for RA Vulkan backend
// written by mudlord.
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
   vTexCoord = Position.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

vec3 rotatex(in vec3 p, float ang) { return vec3(p.x, p.y*cos(ang) - p.z*sin(ang), p.y*sin(ang) + p.z*cos(ang)); }

vec3 rotatey(in vec3 p, float ang) { return vec3(p.x*cos(ang) - p.z*sin(ang), p.y, p.x*sin(ang) + p.z*cos(ang)); }

vec3 rotatez(in vec3 p, float ang) { return vec3(p.x*cos(ang) - p.y*sin(ang), p.x*sin(ang) + p.y*cos(ang), p.z); }

float scene(vec3 p)
{
  float time = float(global.FrameCount)*0.015;
  p = rotatex(p, 0.18*time);
  p = rotatez(p, 0.20*time);
  p = rotatey(p, 0.22*time);

  float d0 = length(max(abs(p) - 0.5, 0.0)) - 0.01 + clamp(sin((p.x +p.y + p.z)*20.0)*0.03, 0.0, 1.0);
  float d1 = length(p) - 0.5;
  return sin(max(d0, -d1));
}

vec3 get_normal(vec3 p)
{
  vec3 eps = vec3(0.01, 0.0, 0.0);
  float nx = scene(p + eps.xyy) - scene(p - eps.xyy);
  float ny = scene(p + eps.yxy) - scene(p - eps.yxy);
  float nz = scene(p + eps.yyx) - scene(p - eps.yyx);
  return normalize(vec3(nx, ny, nz));
}

void main(void)
{

  vec2 p = 2.0 * (vTexCoord.xy*global.OutputSize.xy) / global.OutputSize.xy - 1.0;
  p.x *= global.OutputSize.x / global.OutputSize.y;
  vec3 ro = vec3(0.0, 0.0, 1.7);
  vec3 rd = normalize(vec3(p.x, p.y, -1.4));
  vec3 color = (1.0 - vec3(length(p*0.5)))*0.2;
  vec3 pos = ro;
  float dist = 0.0;
  for (int i = 0; i < 64; i++)
  {
    float d = scene(pos);
    pos += rd*d;
    dist += d;
  }

  if (dist < 100.0)
  {
    vec3 n = get_normal(pos);
    vec3 r = reflect(normalize(pos - ro), n);
    vec3 h = -normalize(n + pos - ro);
    float diff = 1.0*clamp(dot(n, normalize(vec3(1, 1, 1))), 0.0, 1.0);
    float diff2 = 0.2*clamp(dot(n, normalize(vec3(0.7, -1, 0.5))), 0.0, 1.0);
    float diff3 = 0.1*clamp(dot(n, normalize(vec3(-0.7, -0.4, 0.7))), 0.0, 1.0);
    float spec = pow(clamp(dot(h, normalize(vec3(1, 1, 1))), 0.0, 1.0), 50.0);
    float amb = 0.2 + pos.y;
    color = diff*vec3(1, 1, 1) + diff2*vec3(1, 0, 0) + diff3*vec3(1, 0, 1) + spec*vec3(1, 1, 1) + amb*vec3(0.2, 0.2, 0.2);
    color /= dist;
  }
  FragColor = vec4(color, 1.0);
}