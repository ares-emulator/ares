#version 450
// Generators -  Kali - 2013-10-10
// https://www.shadertoy.com/view/Xtf3Rn

// Generators!

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

// "GENERATORS REDUX" by Kali 

// Same fractal as "Ancient Temple" + rotations, improved shading 
// (better coloring, AO and  shadows), some lighting effects, and a path for the camera  
// following a liquid metal ball. 


#define ENABLE_HARD_SHADOWS // turn off to enable faster AO soft shadows 
//#define ENABLE_VIBRATION
//#define ENABLE_POSTPROCESS // Works better on window view rather than full screen


#define RAY_STEPS 70
#define SHADOW_STEPS 50
#define LIGHT_COLOR vec3(.85,.9,1.)
#define AMBIENT_COLOR vec3(.8,.83,1.)
#define FLOOR_COLOR vec3(1.,.7,.9)
#define ENERGY_COLOR vec3(1.,.7,.4)
#define BRIGHTNESS .9
#define GAMMA 1.3
#define SATURATION .85


#define detail .00005
#define t iGlobalTime*.25



vec3 lightdir=normalize(vec3(0.5,-0.3,-1.));
vec3 ambdir=normalize(vec3(0.,0.,1.));
const vec3 origin=vec3(0.,3.11,0.);
vec3 energy=vec3(0.01);
#ifdef ENABLE_VIBRATION
float vibration=sin(iGlobalTime*60.)*.0013;
#else
float vibration=0.;
#endif
float det=0.0;
vec3 pth1;


mat2 rot(float a) {
	return mat2(cos(a),sin(a),-sin(a),cos(a));	
}


vec3 path(float ti) {
return vec3(sin(ti),.3-sin(ti*.632)*.3,cos(ti*.5))*.5;
}

float Sphere(vec3 p, vec3 rd, float r){//A RAY TRACED SPHERE
	float b = dot( -p, rd );
	float inner = b * b - dot( p, p ) + r * r;
	if( inner < 0.0 ) return -1.0;
	return b - sqrt( inner );
}

vec2 de(vec3 pos) {
	float hid=0.;
	vec3 tpos=pos;
	tpos.xz=abs(.5-mod(tpos.xz,1.));
	vec4 p=vec4(tpos,1.);
	float y=max(0.,.35-abs(pos.y-3.35))/.35;
	for (int i=0; i<7; i++) {//LOWERED THE ITERS
		p.xyz = abs(p.xyz)-vec3(-0.02,1.98,-0.02);
		p=p*(2.0+vibration*y)/clamp(dot(p.xyz,p.xyz),.4,1.)-vec4(0.5,1.,0.4,0.);
		p.xz*=mat2(-0.416,-0.91,0.91,-0.416);
	}
	float fl=pos.y-3.013;
	float fr=(length(max(abs(p.xyz)-vec3(0.1,5.0,0.1),vec3(0.0)))-0.05)/p.w;//RETURN A RRECT
	//float fr=length(p.xyz)/p.w;
	float d=min(fl,fr);
	d=min(d,-pos.y+3.95);
	if (abs(d-fl)<.001) hid=1.;
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
	float dist=10.;
	float t1=Sphere((pos-.005*sdir)-pth1,-sdir,0.015);
	if (t1>0. && t1<.5) {
		vec3 sphglowNorm=normalize(pos-t1*sdir-pth1);
		sh=1.-pow(max(.0,dot(sphglowNorm,sdir))*1.2,3.);
	} 
		for (int steps=0; steps<SHADOW_STEPS; steps++) {
			if (totdist<.6 && dist>detail) {
				vec3 p = pos - totdist * sdir;
				dist = de(p).x;
				sh = min( sh, max(50.*dist/totdist,0.0) );
				totdist += max(.01,dist);
			}
		}
	
    return clamp(sh,0.1,1.0);
}


float calcAO( const vec3 pos, const vec3 nor ) {
	float aodet=detail*40.;
	float totao = 0.0;
    float sca = 14.0;
    for( int aoi=0; aoi<5; aoi++ ) {
        float hr = aodet*float(aoi*aoi);
        vec3 aopos =  nor * hr + pos;
        float dd = de( aopos ).x;
        totao += -(dd-hr)*sca;
        sca *= 0.7;
    }
    return clamp( 1.0 - 5.0*totao, 0., 1.0 );
}

float _texture(vec3 p) {
	p=abs(.5-fract(p*10.));
	vec3 c=vec3(3.);
	float es, l=es=0.;
	for (int i = 0; i < 10; i++) { 
			p = abs(p + c) - abs(p - c) - p; 
			p/= clamp(dot(p, p), .0, 1.);
			p = p* -1.5 + c;
			if ( mod(float(i), 2.) < 1. ) { 
				float pl = l;
				l = length(p);
				es+= exp(-1. / abs(l - pl));
			}
	}
	return es;
}

vec3 light(in vec3 p, in vec3 dir, in vec3 n, in float hid) {//PASSING IN THE NORMAL
	#ifdef ENABLE_HARD_SHADOWS
		float sh=shadow(p, lightdir);
	#else
		float sh=calcAO(p,-2.5*lightdir);//USING AO TO MAKE VERY SOFT SHADOWS
	#endif
	float ao=calcAO(p,n);
	float diff=max(0.,dot(lightdir,-n))*sh;
	float y=3.35-p.y;
	vec3 amb=max(.5,dot(dir,-n))*.5*AMBIENT_COLOR;
	if (hid<.5) {
		amb+=max(0.2,dot(vec3(0.,1.,0.),-n))*FLOOR_COLOR*pow(max(0.,.2-abs(3.-p.y))/.2,1.5)*2.;
		amb+=energy*pow(max(0.,.4-abs(y))/.4,2.)*max(0.2,dot(vec3(0.,-sign(y),0.),-n))*2.;
	}
	vec3 r = reflect(lightdir,n);
	float spec=pow(max(0.,dot(dir,-r))*sh,10.);
	vec3 col;
	float energysource=pow(max(0.,.04-abs(y))/.04,4.)*2.;
	if (hid>1.5) {col=vec3(1.); spec=spec*spec;}
	else{
		float k=_texture(p)*.23+.2; 
		k=min(k,1.5-energysource);
		col=mix(vec3(k,k*k,k*k*k),vec3(k),.3);
		if (abs(hid-1.)<.001) col*=FLOOR_COLOR*1.3;
	}
	col=col*(amb+diff*LIGHT_COLOR)+spec*LIGHT_COLOR;	
	if (hid<.5) { 
		col=max(col,energy*2.*energysource);
	}
	col*=min(1.,ao+length(energy)*.5*max(0.,.1-abs(y))/.1);
	return col;
}

vec3 raymarch(in vec3 from, in vec3 dir) 

{
	float ey=mod(t*.5,1.);
	float glow,eglow,ref,sphdist,totdist=glow=eglow=ref=sphdist=0.;
	vec2 d=vec2(1.,0.);
	vec3 p, col=vec3(0.);
	vec3 origdir=dir,origfrom=from,sphNorm;
	
	//FAKING THE SQUISHY BALL BY MOVING A RAY TRACED BALL
	vec3 wob=cos(dir*500.0*length(from-pth1)+(from-pth1)*250.+iGlobalTime*10.)*0.0005;
	float t1=Sphere(from-pth1+wob,dir,0.015);
	float tg=Sphere(from-pth1+wob,dir,0.02);
	if(t1>0.){
		ref=1.0;from+=t1*dir;sphdist=t1;
		sphNorm=normalize(from-pth1+wob);
		dir=reflect(dir,sphNorm);
	} 
	else if (tg>0.) { 
		vec3 sphglowNorm=normalize(from+tg*dir-pth1+wob);
		glow+=pow(max(0.,dot(sphglowNorm,-dir)),5.);
	};
	
	for (int i=0; i<RAY_STEPS; i++) {
		if (d.x>det && totdist<3.0) {
			p=from+totdist*dir;
			d=de(p);
			det=detail*(1.+totdist*60.)*(1.+ref*5.);
			totdist+=d.x; 
			energy=ENERGY_COLOR*(1.5+sin(iGlobalTime*20.+p.z*10.))*.25;
			if(d.x<0.015)glow+=max(0.,.015-d.x)*exp(-totdist);
			if (d.y<.5 && d.x<0.03){//ONLY DOING THE GLOW WHEN IT IS CLOSE ENOUGH
				float glw=min(abs(3.35-p.y-ey),abs(3.35-p.y+ey));//2 glows at once
				eglow+=max(0.,.03-d.x)/.03*
				(pow(max(0.,.05-glw)/.05,5.)
				+pow(max(0.,.15-abs(3.35-p.y))/.15,8.))*1.5;
			}
		}
	}
	float l=pow(max(0.,dot(normalize(-dir.xz),normalize(lightdir.xz))),2.);
	l*=max(0.2,dot(-dir,lightdir));
	vec3 backg=.5*(1.2-l)+LIGHT_COLOR*l*.7;
	backg*=AMBIENT_COLOR;
	if (d.x<=det) {
		vec3 norm=normal(p-abs(d.x-det)*dir);//DO THE NORMAL CALC OUTSIDE OF LIGHTING (since we already have the sphere normal)
		col=light(p-abs(d.x-det)*dir, dir, norm, d.y)*exp(-.2*totdist*totdist); 
		col = mix(col, backg, 1.0-exp(-1.*pow(totdist,1.5)));
	} else { 
		col=backg;
	}
	vec3 lglow=LIGHT_COLOR*pow(l,30.)*.5;
	col+=glow*(backg+lglow)*1.3;
	col+=pow(eglow,2.)*energy*.015;
	col+=lglow*min(1.,totdist*totdist*.3);
	if (ref>0.5) {
		vec3 sphlight=light(origfrom+sphdist*origdir,origdir,sphNorm,2.);
		col=mix(col*.3+sphlight*.7,backg,1.0-exp(-1.*pow(sphdist,1.5)));
	}
	return col; 
}

vec3 move(inout mat2 rotview1,inout mat2 rotview2) {
	vec3 go=path(t);
	vec3 adv=path(t+.7);
	vec3 advec=normalize(adv-go);
	float an=atan(advec.x,advec.z);
	rotview1=mat2(cos(an),sin(an),-sin(an),cos(an));
		  an=advec.y*1.7;
	rotview2=mat2(cos(an),sin(an),-sin(an),cos(an));
	return go;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	pth1 = path(t+.3)+origin+vec3(0.,.01,0.);
	vec2 uv = fragCoord.xy / iResolution.xy*2.-1.;
	vec2 uv2=uv;
#ifdef ENABLE_POSTPROCESS
	uv*=1.+pow(length(uv2*uv2*uv2*uv2),4.)*.07;
#endif
	uv.y*=iResolution.y/iResolution.x;
#ifdef MOUSE
	vec2 mouse=(iMouse.xy/iResolution.xy-.5)*3.;
	if (iMouse.z<1.) mouse=vec2(0.);
#else
   vec2 mouse = vec2(0.0, 0.0);
#endif
	mat2 rotview1, rotview2;
	vec3 from=origin+move(rotview1,rotview2);
	vec3 dir=normalize(vec3(uv*.8,1.));
	dir.yz*=rot(mouse.y);
	dir.xz*=rot(mouse.x);
	dir.yz*=rotview2;
	dir.xz*=rotview1;
	vec3 color=raymarch(from,dir); 
	color=clamp(color,vec3(.0),vec3(1.));
	color=pow(color,vec3(GAMMA))*BRIGHTNESS;
	color=mix(vec3(length(color)),color,SATURATION);
#ifdef ENABLE_POSTPROCESS
	vec3 rain=pow(texture(iChannel0,uv2+iGlobalTime*7.25468).rgb,vec3(1.5));
	color=mix(rain,color,clamp(iGlobalTime*.5-.5,0.,1.));
	color*=1.-pow(length(uv2*uv2*uv2*uv2)*1.1,6.);
	uv2.y *= iResolution.y / 360.0;
	color.r*=(.5+abs(.5-mod(uv2.y     ,.021)/.021)*.5)*1.5;
	color.g*=(.5+abs(.5-mod(uv2.y+.007,.021)/.021)*.5)*1.5;
	color.b*=(.5+abs(.5-mod(uv2.y+.014,.021)/.021)*.5)*1.5;
	color*=.9+rain*.35;
#endif
	fragColor = vec4(color,1.);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
