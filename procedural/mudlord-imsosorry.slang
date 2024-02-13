#version 450
// im so sorry 
// done for Flashback 2019 cracktro
// written by mudlord.

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
   float SPEED;
} global;

#pragma parameter SPEED "Animation Speed" 0.5 0.0 1.0 0.05
#define time global.FrameCount / 10. * global.SPEED

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

float SuperFormula(float phi, float a, float b, float m, float n1, float n2, float n3)
{
	return pow((pow(abs(cos(m*phi/4.0)/a),n2) + pow(abs(sin(m*phi/4.0)/b), n3)), -(1.0/n1));
}

vec2 scene(vec3 p)
{
	p = rotatex(p, 0.18*time);
	p = rotatez(p, 0.20*time);
	p = rotatey(p, 0.22*time);
    float d=length(p);
	float sn=p.z/d;
	vec4 w =vec4(12.,6.,6.,16.);
	float r1=SuperFormula(atan(p.y,p.x),1.0+0.0025*sin(time),1.0,w.x,w.y,w.z,w.w);
	float r2=SuperFormula(asin(sn),1.0,1.0,w.x,w.y,w.z,w.w);
	d-=r2*sqrt(r1*r1*(1.0-sn*sn)+sn*sn);
	return vec2(d,1.0);
}
vec2 castRay( in vec3 ro, in vec3 rd )
{
    float t = 0.0;
    float m = -1.0;
    for( int i=0; i<64; i++ )
    {
	    vec2 res = scene( ro+rd*t );
      t += res.x;
	    m = res.y;
      if (res.x <= 0.001)return vec2( t, m );
    }
    return vec2( t, m );
}
vec3 get_normal(vec3 p)
{
	vec3 eps = vec3(0.11, 0.0, 0.0);
	float nx = scene(p + eps.xyy).x - scene(p - eps.xyy).x;
	float ny = scene(p + eps.yxy).x - scene(p - eps.yxy).x;
	float nz = scene(p + eps.yyx).x - scene(p - eps.yyx).x;
	return normalize(vec3(nx, ny, nz));
}

float hex(vec2 p) {
  p.x *= 0.57735*2.0;
	p.y += mod(floor(p.x), 2.0)*0.5;
	p = abs((mod(p, 1.0) - 0.5));
	return abs(max(p.x*1.5 + p.y, p.y*2.0) - 1.0);
}
vec2 noise(vec2 t)
{
t=vec2(dot(t,vec2(127.1,311.7)),dot(t,vec2(269.5,183.3)));
t=fract(sin(t)*43758.5);
return t;
}

float voronoi(vec2 v)
	{
	vec2 r=floor(v);
	vec2 e=fract(v);
	float f=8.;
	for(float u=-1.;u<=1.;u++)
	for(float m=-1.;m<=1.;m++)
	{
	vec2 g=vec2(m,u);
	vec2 d=noise(r+g);
	d=.1+.5*sin(time+4.2831*d);
	vec2 s=g+d-e;
	float o=dot(s,s);
	f=min(f,o);
	}
	return f;
}

float getBorder( in vec2 p )
{
    float dis = voronoi( p );
    
    return 1.0 - smoothstep( 0.0, .26, dis );
}

void main(void)
{
	vec2 p = 2.0 * (vTexCoord.xy*global.OutputSize.xy) / global.OutputSize.xy - 1.0;
	p.x *= global.OutputSize.x / global.OutputSize.y;
	vec3 ro = vec3(-0.5, 0., 4.);
	vec3 rd = normalize(vec3(p, -1.4));
	vec3 pos = ro;
	float dist = 0.0;
	vec2 result_raymarch = castRay(ro,rd);
	dist = result_raymarch.x;

		float f=1-hex(p*8.);
    float voron = getBorder(1.8*p)*sin(f);
    vec3 color2=(f*voron)*vec4(43./255.,73./255.,112./255.,0.0).rgb;
	vec3 color = color2.rgb;
	if (dist < 20.)
	{
	    vec3 pos = ro + dist*rd;
		vec3 n = get_normal(pos);
		vec3 r = reflect(normalize(pos - ro), n);
		vec3 h = -normalize(n + pos - ro);
		vec3 gold = vec3(170./255.,127./255.,57./255.);
		float diff = 1.0*clamp(dot(n, normalize(vec3(1, 1, 1))), 0.0, 1.0);
		float diff2 = 0.2*clamp(dot(n, normalize(vec3(0.7, -1, 0.5))), 0.0, 1.0);
		float diff3 = 0.1*clamp(dot(n, normalize(vec3(-0.7, -0.4, 0.7))), 0.0, 1.0);
		float spec = pow(clamp(dot(h, normalize(vec3(1, 223./255., 170./255.))), 0.0, 1.0), 50.0);
		float amb = 2.5;
		color = diff*vec3(1, 1, 1) + diff2*gold + diff3*gold + spec*vec3(1, 0, 0) + amb*gold;
		color /= dist;
	}
	FragColor = vec4(color, 1.0);
}
