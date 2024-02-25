#version 450
// Sound - Digital Ambience -  srtuss - 2014-07-31
// https://www.shadertoy.com/view/MdXXW2

// i tried to rebuild the technique that creates the ambience sounds in my game project. it's based on additive synthesis. go ahead and "listen" to various textures

// srtuss, 2014

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

vec2 rotate(vec2 p, float a)
{
	return vec2(p.x * cos(a) - p.y * sin(a), p.x * sin(a) + p.y * cos(a));
}

#define ITS 8

vec2 circuit(vec3 p)
{
	p = mod(p, 2.0) - 1.0;
	float w = 1e38;
	vec3 cut = vec3(1.0, 0.0, 0.0);
	vec3 e1 = vec3(-1.0);
	vec3 e2 = vec3(1.0);
	float rnd = 0.23;
	float pos, plane, cur;
	float fact = 0.9;
	float j = 0.0;
	for(int i = 0; i < ITS; i ++)
	{
		pos = mix(dot(e1, cut), dot(e2, cut), (rnd - 0.5) * fact + 0.5);
		plane = dot(p, cut) - pos;
		if(plane > 0.0)
		{
			e1 = mix(e1, vec3(pos), cut);
			rnd = fract(rnd * 9827.5719);
			cut = cut.yzx;
		}
		else
		{
			e2 = mix(e2, vec3(pos), cut);
			rnd = fract(rnd * 15827.5719);
			cut = cut.zxy;
		}
		j += step(rnd, 0.2);
		w = min(w, abs(plane));
	}
	return vec2(j / float(ITS - 1), w);
}

float scene(vec3 p)
{
	vec2 cir = circuit(p);
	return exp(-100.0 * cir.y) + pow(cir.x * 1.8 * (sin(p.z * 10.0 + iGlobalTime * -5.0 + cir.x * 10.0) * 0.5 + 0.5), 8.0);
}

float nse(float x)
{
    return fract(sin(x * 297.9712) * 90872.2961);
}

float nseI(float x)
{
    float fl = floor(x);
    return mix(nse(fl), nse(fl + 1.0), smoothstep(0.0, 1.0, fract(x)));
}

float fbm(float x)
{
    return nseI(x) * 0.5 + nseI(x * 2.0) * 0.25 + nseI(x * 4.0) * 0.125;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	vec2 suv = uv;
	uv = 2.0 * uv - 1.0;
	uv.x *= iResolution.x / iResolution.y;
	vec3 ro = vec3(0.0, iGlobalTime * 0.2, 0.1);
	vec3 rd = normalize(vec3(uv, 0.9));
	ro.xz = rotate(ro.xz, iGlobalTime * 0.1);
	ro.xy = rotate(ro.xy, 0.2);
	rd.xz = rotate(rd.xz, iGlobalTime * 0.2);
	rd.xy = rotate(rd.xy, 0.2);
	float acc = 0.0;
	vec3 r = ro + rd * 0.5;
	for(int i = 0; i < 50; i ++)
	{
		acc += scene(r + nse(r.x) * 0.03);
		r += rd * 0.015;
	}
	vec3 col = pow(vec3(acc * 0.04), vec3(0.2, 0.6, 2.0) * 8.0) * 2.0;
	//col -= exp(length(suv - 0.5) * -2.5 - 0.2);
    col = clamp(col, vec3(0.0), vec3(1.0));
    col *= fbm(iGlobalTime * 6.0) * 2.0;
	col = pow(col, vec3(1.0 / 2.2));
	//col = clamp(col, vec3(0.0), vec3(1.0));
	fragColor = vec4(col, 1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
