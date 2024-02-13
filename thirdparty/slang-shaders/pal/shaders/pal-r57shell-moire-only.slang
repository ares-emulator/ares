#version 450

// NES CRT simulation (PAL)
// by r57shell
// https://www.shadertoy.com/view/MlsXWM
// WARNING: NOOB HERE :)
// Stream RU
// http://sc2tv.ru/channel/r57shell
// http://cybergame.tv/r57shell
// http://twitch.tv/elektropage

// some details
// http://forums.nesdev.com/viewtopic.php?f=3&t=12788&start=15

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
   vTexCoord = TexCoord * 1.00004;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define iChannel0 Source
#define iResolution params.SourceSize
#define fragCoord vec2(vTexCoord.xy * params.OutputSize.xy)

vec2 size = params.SourceSize.xy;//vec2(320.,240.);
const float pi = 3.141592654;

vec3 monitor(vec2 p)
{
	vec2 pos = floor(p*size);
	vec2 uv = vTexCoord;// floor(pos)/size;
   	vec4 res = texture(iChannel0, uv);
    vec3 yuv = res.xyz*mat3(
        0.2126, 0.7152, 0.0722,
		-0.09991, -0.33609, 0.436,
		0.615, -0.55861, -0.05639);
    float alpha = (floor(p.x*size.x*4.)/2.0)*pi;
    vec2 sincv = vec2(cos(alpha), sin(alpha));
    if (mod(pos.y + 5.,4.) < 2.)
     sincv.x = -sincv.x;
    if (mod(pos.y, 4.) >= 2.)
     sincv.y = -sincv.y;
    float mc = 1.+dot(sincv, yuv.zy)/yuv.x;

    /*vec3 rgb = vec3(
        yuv.x + 1.28033 * yuv.z,
		yuv.x - 0.21482 * yuv.y - 0.38059 * yuv.z,
		yuv.x + 2.12798 * yuv.y);*/
    return res.xyz*mc;
}

// pos (left corner, sample size)
vec4 monitor_sample(vec2 p, vec2 tex_sample)
{
	// linear interpolation was...
    // now other thing.
    // http://imgur.com/m8Z8trV
    // AT LAST IT WORKS!!!!
    vec2 next = vec2(.25,1.)/size;
    vec2 f = fract(vec2(4.,1.)*size*p);
    tex_sample *= vec2(4.,1.)*size;
    vec2 l;
    vec2 r;
    if (f.x+tex_sample.x < 1.)
    {
       	l.x = f.x+tex_sample.x;
        r.x = 0.;
    }
    else
    {
        l.x = 1.-f.x;
        r.x = min(1.,f.x+tex_sample.x-1.);
    }
    if (f.y+tex_sample.y < 1.)
    {
       	l.y = f.y+tex_sample.y;
        r.y = 0.;
    }
    else
    {
        l.y = 1.-f.y;
        r.y = min(1.,f.y+tex_sample.y-1.);
    }
    vec3 top = mix(monitor(p),monitor(p+vec2(next.x,0.)),r.x/(l.x+r.x));
    vec3 bottom = mix(monitor(p+vec2(0.,next.y)),monitor(p+next),r.x/(l.x+r.x));
   	return vec4(mix(top,bottom,r.y/(l.y+r.y)),1.0);
    //difference should be only on border of pixels
    //return vec4((mix(top,bottom,r.y/(l.y+r.y)) - monitor(p))*2.+0.5,1.0);
}

void main()
{
	float zoom = 1.;
	FragColor = monitor_sample(vTexCoord, vec2(1.0));
}
