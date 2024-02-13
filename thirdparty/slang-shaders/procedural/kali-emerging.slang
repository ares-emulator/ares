#version 450
// Emerging -  Kali - 2015-03-10
// https://www.shadertoy.com/view/XlfGWI

// An emerging fractal tower. You can use the mouse. Or not.

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

#define ENABLE_HARD_SHADOWS // turn off to enable faster AO soft shadows 


#define RAY_STEPS 80
#define SHADOW_STEPS 50
#define LIGHT_COLOR vec3(.97,.92,.82)
#define AMBIENT_COLOR vec3(.57,.55,.6)
#define FLOOR_COLOR vec3(.35,.25,.2)
#define ENERGY_COLOR vec3(1.,.7,.4)
#define BRIGHTNESS 1.5
#define GAMMA 1.2
#define SATURATION .9



#define detail .00003
#define t iGlobalTime*.1


float cc,ss;
vec3 lightdir=normalize(vec3(0.5,-0.4,-1.));
vec3 ambdir=normalize(vec3(0.,0.,1.));
const vec3 origin=vec3(0.,3.11,0.);
float det=0.0;
vec3 pth1;


float smin( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}


mat2 rot(float a) {
	return mat2(cos(a),sin(a),-sin(a),cos(a));	
}


vec3 path(float ti) {
return vec3(0.,2.5,0.)+vec3(cos(ti)*.9,cos(ti*.5),sin(ti)*.8);
}


vec4 formula (vec4 p) {
	p.y-=t*.25;
    p.y=abs(3.-mod(p.y-t,6.));
    for (int i=0; i<6; i++) {
		p.xyz = abs(p.xyz)-vec3(.0,1.,.0);
		p=p*1.6/clamp(dot(p.xyz,p.xyz),.2,1.)-vec4(0.4,1.5,0.4,0.);
		p.xz*=mat2(cc,ss,-ss,cc);
	}
	return p;
}


float texture1(vec3 p) {
    p=abs(1.-mod(p,2.));
    vec3 c=vec3(3.);
	float es=1000., l=0.;
	for (int i = 0; i < 8; i++) { 
			p = abs(p + c) - abs(p - c) - p; 
			p/= clamp(dot(p, p), .25, 1.);
			p = p* -1.5 + c;
        	es=min(min(abs(p.x),abs(p.y)),es);
	}
	return es*es;
}

float texture2 (vec3 p) {
	//p.xz=abs(.75-mod(p.xz,1.5));
    p=formula(vec4(p,0.)).xyz;
    return .13+clamp(pow(max(0.,1.-max(abs(p.x),abs(p.z))),2.)*2.,.1,.7);
}


vec2 de(vec3 pos) {
	float aa=smoothstep(0.,1.,clamp(cos(t-pos.y*.4)*1.5,0.,1.))*3.14159;
    cc=cos(aa);ss=sin(aa);
    float hid=0.;
	vec3 tpos=pos;
	//tpos.xz=abs(1.5-mod(tpos.xz,3.))-1.5;
	vec4 p=vec4(tpos,1.);
	float y=max(0.,.3-abs(pos.y-3.3))/.3;
    p=formula(p);
    float fl=pos.y-3.7-length(sin(pos.xz*60.))*.01;
	float fr=max(abs(p.z/p.w)-.01,length(p.zx)/p.w-.002);
	float bl=max(abs(p.x/p.w)-.01,length(p.zy)/p.w-.0005);
    fr=smin(bl,fr,.02);
	fr*=.9;
    //float fr=length(p.xyz)/p.w;
    fl-=(length(p.xz)*.005+length(sin(pos*3.+t*5.))*.15);
    fl*=.9;
	float d=smin(fl,fr,.7);
    if (abs(d-fl)<.2) {
        hid=1.;
    }
    return vec2(d,hid);
}


vec3 normal(vec3 p) {
	vec3 e = vec3(0.0,det,0.0);
	
	return normalize(vec3(
			de(p+e.yxx).x-de(p-e.yxx).x,
			de(p+e.xyx).x-de(p-e.xyx).x,
			de(p+e.xxy).x-de(p-e.xxy).x
			)
		);	
}

float shadow(vec3 pos, vec3 sdir) {//THIS ONLY RUNS WHEN WITH HARD SHADOWS
	float sh=1.0;
	float totdist =2.0*det;
	float dist=5.;
		for (int steps=0; steps<SHADOW_STEPS; steps++) {
			if (totdist<4. && dist>detail) {
				vec3 p = pos - totdist * sdir;
				dist = de(p).x;
				sh = min( sh, max(20.*dist/totdist,0.0) );
				totdist += max(.01,dist);
			}
		}
	
    return clamp(sh,0.1,1.0);
}


float calcAO( const vec3 pos, const vec3 nor ) {
	float aodet=detail*75.;
	float totao = 0.0;
    float sca = 10.0;
    for( int aoi=0; aoi<5; aoi++ ) {
        float hr = aodet*float(aoi*aoi);
        vec3 aopos =  nor * hr + pos;
        float dd = de( aopos ).x;
        totao += -(dd-hr)*sca;
        sca *= 0.7;
    }
    return clamp( 1.0 - 5.0*totao, 0., 1. );
}


vec3 light(in vec3 p, in vec3 dir, in vec3 n, in float hid) {//PASSING IN THE NORMAL
	#ifdef ENABLE_HARD_SHADOWS
		float sh=shadow(p, lightdir);
	#else
		float sh=calcAO(p,-2.5*lightdir);//USING AO TO MAKE VERY SOFT SHADOWS
	#endif
	float ao=calcAO(p,n);
	float diff=max(0.,dot(lightdir,-n))*sh*.95;
	float y=3.16-p.y;
	vec3 amb=max(.6,dot(dir,-n))*.7*AMBIENT_COLOR;
	vec3 r = reflect(lightdir,n);
	float spec=pow(max(0.,dot(dir,-r))*sh,15.)*.5;
	vec3 col;
	float energysource=pow(max(0.,.1-abs(y))/.1,3.)*1.5;
		float k=texture2(p); 
		col=mix(vec3(k,k*k,k*k*k)*.9+.1,vec3(k)*1.5,.4);
		if (abs(hid-1.)<.001) col=FLOOR_COLOR;
	col=col*(amb*ao+diff*(.3+ao*.5)*LIGHT_COLOR)+spec*LIGHT_COLOR;	
	return col;
}

vec3 raymarch(in vec3 from, in vec3 dir) 

{
	float ey=mod(t*.5,1.);
	float glow,eglow,ref,sphdist,totdist=glow=eglow=ref=sphdist=0.;
	vec2 d=vec2(1.,0.);
	vec3 p, col=vec3(0.);
	vec3 origdir=dir,origfrom=from,sphNorm;
	
    for (int i=0; i<RAY_STEPS; i++) {
		if (d.x>det && totdist<6.0) {
			p=from+totdist*dir;
			d=de(p);
			det=detail*(1.+totdist*60.)*(1.+ref*5.);
			totdist+=max(detail,d.x); 
			if (d.y<.5) glow+=max(0.,.02-d.x)/.02;
		}
	}
	vec3 ov=normalize(vec3(1.,.5,1.));
	vec3 sol=dir+lightdir;
    float l=pow(max(0.,dot(normalize(-dir*ov),normalize(lightdir*ov))),1.5)+sin(atan(sol.x,sol.y)*20.+length(from)*50.)*.002;
    totdist=min(5.9,totdist);
    p=from+dir*(totdist-detail);
    vec3 backg=.4*(1.2-l)+LIGHT_COLOR*l*.75;
	backg*=AMBIENT_COLOR*(1.-max(0.2,dot(normalize(dir),vec3(0.,1.,0.)))*.2);
	float fondo=0.;
	vec3 pp=p*.5+sin(t*2.)*.5;
    for (int i=0; i<15; i++) {
        fondo+=clamp(0.,1.,texture1(pp+dir*float(i)*.02))*max(0.,1.-exp(-.03*float(i)));
    }
    vec3 backg2=backg*(1.+fondo*(LIGHT_COLOR)*.75);
    if (d.x<.01) {
        vec3 norm=normal(p);
		col=mix(light(p-abs(d.x-det)*dir, dir, norm, d.y),backg,1.-exp(-.3*totdist*totdist)); 
		col = mix(col, backg2, 1.0-exp(-.02*pow(abs(totdist),2.)));
	} else { 
		col=backg2;
	}
	vec3 lglow=LIGHT_COLOR*pow(abs(l),30.)*.5;
    col+=glow*(.3+backg+lglow)*.005;
	col+=lglow*min(1.,totdist*totdist*.03)*1.2;
	return col; 
}

vec3 move(inout mat2 rotview1,inout mat2 rotview2) {
	vec3 go=path(t);
	vec3 adv=path(t+.5);
	vec3 advec=normalize(adv-go);
	float an=atan(advec.x,advec.z);
	rotview1=mat2(cos(an),sin(an),-sin(an),cos(an));
		  an=advec.y*1.5-.2;
	rotview2=mat2(cos(an),sin(an),-sin(an),cos(an));
	return go;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    pth1 = path(t+.3)+origin+vec3(0.,.01,0.);
	vec2 uv = fragCoord.xy / iResolution.xy*2.-1.;
    vec2 uv2=uv;
	uv.y*=iResolution.y/iResolution.x;
#ifdef MOUSE
	vec2 mouse=(iMouse.xy/iResolution.xy-.5)*3.;
	if (iMouse.z<1.) mouse=vec2(0.);
##else
	vec2 mouse=(0.0 / iResolution.xy-.5)*3.;
#endif
	mat2 rotview1, rotview2;
	vec3 from=origin+move(rotview1,rotview2);
	vec3 dir=normalize(vec3(uv*1.5,1.));
	dir.yz*=rot(mouse.y);
	dir.xz*=rot(mouse.x-1.);
	dir.yz*=rotview2;
	dir.xz*=rotview1;
	vec3 color=raymarch(from,dir); 
	color=clamp(color,vec3(.0),vec3(1.));
	color=pow(abs(color),vec3(GAMMA))*BRIGHTNESS;
	color=mix(vec3(length(color)),color,SATURATION);
	color*=1.2-length(pow(abs(uv2),vec2(4.)))*.3;
	float fadein=clamp(iGlobalTime-.5,0.,1.);
    fragColor = vec4(color*vec3(.93,.95,.91),1.)*fadein;
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
