#version 450
// Somewhere in 1993 -  nimitz - 2014-10-10
// https://www.shadertoy.com/view/Md2XDD

// Experiments in making low poly terrain.

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

//Somewhere in 1993 by nimitz (twitter: @stormoid)

#define PALETTE 6.8

//3 to 5 works best
#define TERRAIN_COMPLEXITY 4.
#define ITR 100
#define FAR 700.
#define time mod(iGlobalTime,500.)

mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,-s,s,c);}
float smoothfloor(const in float x, const in float w)
{
    return floor(x)+smoothstep(w, 1.-w,fract(x));
}

vec3 enpos()
{
    return vec3(sin(time)*100.+50.,sin(time)*30.+30.,300.+sin(time*.9+sin(time*0.88+0.2))*100.);
}

//--------------------------------------------------------
//---------------------------HUD--------------------------
//--------------------------------------------------------

float square(in vec2 p){ return max(abs(p.x),abs(p.y));}
float loz(in vec2 p){ return abs(p.x)+abs(p.y);}

//from Dave (https://www.shadertoy.com/view/4djSRW)
vec2 hash2(float p)
{
	vec2 p2  = fract(p * vec2(5.3983, 5.4427));
    p2 += dot(p2.yx, p2.xy +  vec2(21.5351, 14.3137));
	return fract(vec2(p2.x * p2.y * 95.4337, p2.x * p2.y * 97.597));
}

float line( in vec2 a, in vec2 b, in vec2 p )
{
	vec2 pa = p - a, ba = b - a;
	float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
	return length( pa - ba*h );
}

float crosshair(in vec2 p , in float tk, in float rt)
{
    float d = abs(p.x)+abs(p.y);
    float a = atan(p.y,p.x);
    float rz = smoothstep(0.03*tk,.04*tk,abs(d-0.5));
    d = sin(a*3.+1.59-time*3.5-rt);
    rz += smoothstep(0.0,.07*tk,d);
    return rz;
}

//inspired by otaviogood "runes" (https://www.shadertoy.com/view/MsXSRn)
float text2(in vec2 p)
{
    p = (p+vec2(1.75,-.8))*7.;
    p.x *= 1.5;
    float sd = floor(time*8.);
    vec2 p1 = vec2(0), p2 = hash2(sd);
    float d= 1.;
    vec2 fl = vec2(2.,2.);
    for(float i=0.;i<7.;i++)
    {
    	if(hash2(sd+i+10.).x<0.3)continue;
        p1 = hash2(i+sd);
    	p2 = hash2(i+sd+1.);
		p1 = (floor(p1*fl) + .5)/fl;
		p2 = (floor(p2*fl) + .5)/fl;
        if (p1 == p2) p2 = vec2(.5);
    	d = min(line(p1, p2, p), d);        
        p1 = p2;
    	p2 = hash2(i+sd+3.);
		p2 = (floor(p2*fl) + .5)/fl;
    	d = min(line(p1, p2, p), d);
        p1 = p2;
    	p2 = hash2(i+sd+5.);
		p2 = (floor(p2*fl) + .5)/fl;
        if (p1 == p2)
        {
            p2 = hash2(i+sd+7.);
			p2 = (floor(p2*fl) + .5)/fl;
        }
    	d = min(line(p1,p2,p),d);
        p.x -= .8;
    }

    d = smoothstep(0.03, .08,d);
    return d;
}

vec3 makeHud(in vec2 p, in float seek)
{
    float sk1 = smoothstep(0.99, 1., seek);
    float sk2 = step(1.-sk1, .5);
    //lens deformation
    float ll = abs(p.x)+abs(p.y)*0.25;
    p *= ll * -.3+1.29;
    p *= 2.;
    vec3 col = vec3(0);
    float d= 1.;
    //crosshairs
    float rz = crosshair(p*1.1, .9,1.+sk1);
    rz = min(rz,crosshair(p*2.7,2., -time*6.5-1.1-sk1));
    //minimap (top right)
    float d2 = square(p+vec2(-1.45, -0.67))+0.02;
    d = smoothstep(0.3,0.31,d2);
    d = max(d,smoothstep(0.35,.55,min(sin(p.x*80.+1.9),sin(p.y*80.+time*15.))+1.4));
    d = min(d,smoothstep(0.002,0.009,abs(d2-0.3)));
    vec3 enp = enpos()/1000.;
    enp.z = 1.-enp.z;
    float en = smoothstep(0.025, 0.033, loz(enp.xz+p-vec2(1.47, 1.4))) ;
    en += mod(floor(time*2.5), 2.);
    d = min(d,en);
    rz = min(d,rz);
    //text (top left)
    rz= min(rz,text2(p));
    //altitude bars
    d = min(rz,sin(p.y*100.+sin(time)*20.)*3.+3.);
    d2 = max(d,(p.x+0.59)*200.);
    d2 = max(d2,-(p.x+0.66)*200.);
    float d3 = max(d,(p.x-0.66)*200.);
    d3 = max(d3,-(p.x-.59)*200.);
    d2 = min(d2,d3);
    d2 += smoothstep(0.59, .6, -p.y);
    d2 += smoothstep(0.59, .6, p.y);
    rz = min(rz,d2);    
    //bottom left "status"
    float num = mod(floor(time*12.),12.);
    vec2 p2 = p+vec2(-1.32,.94);
    d = 1.;
    for(float i=0.;i<5.;i++)
    {
        d = min(d,length(p2)+float(num==i));
    	p2.x -= 0.1;
    }
    d = smoothstep(0.023,.03,d);
    rz = min(d,rz);
    
    vec3 hcol = (sin(vec3(0.35,0.4,0.48)*(3.35)*PALETTE)*0.5+.5);
    hcol.gb -= sk2;
    hcol.r += sk2;
    return hcol*(1.-rz);
}

//--------------------------------------------------------
//--------------------------------------------------------
//--------------------------------------------------------

float tri(in float x)
{
    return abs(fract(x)-0.5);
}

mat2 m2 = mat2( 0.80,  0.60, -0.60,  0.80 );
float tnoise(in vec2 p)
{
    p*=.008;
    float z=2.;
	float rz = 0.;
	for (float i= 1.;i < TERRAIN_COMPLEXITY;i++ )
	{
        rz+= tri(p.x+tri(p.y*1.))/z;
		z = z*2.;
		p = p*1.8;
        p*= m2;
	}
	return rz*9.;
}

float oct(in vec3 p){ return dot(vec3(0.5773),abs(p));}
vec2 ou( vec2 d1, vec2 d2 ){return (d1.x<d2.x) ? d1 : d2;}

vec3 roty(vec3 p, float a)
{
    float s = sin(a), c = cos(a);
    return vec3(c*p.x + s*p.z, p.y, -s*p.x + c*p.z);
}

vec2 map(vec3 p)
{   
    //terrain
    vec2 d = vec2(6.*tnoise(p.xz)+p.y+20.+(tri(p.z*0.001)-0.4)*22.,1.);
    //xlog(x) seems to work nicely for a valley
    d.x -= abs(p.x*0.5*log(abs(p.x)))*0.05-8.;
    //flat water
    d = ou(d,vec2(p.y+30., 2.));
    //"enemy"
    vec3 enp = enpos();
    enp.z += time*50.;
    d = ou(d,vec2((oct(roty(p-enp, time*2.5))-6.)*0.66,8.));
    
	return d;
}

vec2 march(in vec3 ro, in vec3 rd)
{
	float precis = .1;
    float h=precis*2.0;
    float d = 0.;
    float c = 1.;
    for( int i=0; i<ITR; i++ )
    {
        if( abs(h)<precis || d>FAR ) break;
        d += h;
	    vec2 res = map(ro+rd*d);
        h = res.x*1.4;
        c = res.y;
    }
	return vec2(d,c);
}

vec3 normal(const in vec3 p)
{  
    vec2 e = vec2(-1., 1.)*.1;
	return normalize(e.yxx*map(p + e.yxx).x + e.xxy*map(p + e.xxy).x + 
					 e.xyx*map(p + e.xyx).x + e.yyy*map(p + e.yyy).x );   
}

//(from eiffie, who thought it was from iq, dont know who actually wrote it)
float segm(vec3 ro, vec3 rd, vec3 p1, vec3 p2)
{
	vec3 p = p1-ro;
	vec3 di = p2-ro-p;
	float proj = dot(rd, di);
	float m = clamp((dot(rd,p)*proj-dot(p,di))/(dot(di,di)-proj*proj), 0., 1.);
	p += di*m;
	p = dot(p, rd)*rd-p;
    return smoothstep(0.9985,.999,1.-dot(p,p));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	vec2 p = fragCoord.xy/iResolution.xy-0.5;
    vec2 bp = p+0.5;
	p.x*=iResolution.x/iResolution.y;
	vec2 um = vec2(0);
    um.x = 0.5+(smoothstep(-2.,2.,sin(time*.7-0.1))-0.5)*.1;
    um.y = sin(time+1.)*0.02;
	
    //camera
    vec3 ro = vec3((smoothstep(-2., 2., sin(time*0.7+1.57))-0.5)*50., sin(time)*5.-1., time*50.);
    um.x *= 3.;
    vec3 eye = normalize(vec3(cos(um.x),um.y*5.,sin(um.x)));
    vec3 right = normalize(vec3(cos(um.x+1.5708),0.,sin(um.x+1.5708)));
    mat2 ori = mm2( smoothstep(-.5,.5,sin(time*0.7+0.78))-.5 + smoothfloor(time*0.04,.45)*6.28 );
    right.xy *= ori;
    vec3 up = normalize(cross(right,eye));
	vec3 rd=normalize((p.x*right+p.y*up)*.75+eye);
	
    vec3 bg = sin(vec3(0.35,0.4,0.48)*11.3*PALETTE)*0.5+.5;
    vec3 col = bg*floor(-rd.y*50.+6.)*0.06;
    
    //march
	vec2 rz = march(ro,rd);
    if ( rz.x < FAR )
    {
        vec3 pos = ro+rz.x*rd;
        vec3 nor = normal( pos );
        vec3 ligt = normalize(vec3(-.7,0.2, 0.1));
        float dif = clamp(dot(nor, ligt), 0., 1.);
        float fre = pow(clamp(1. + dot(nor, rd), 0., 1.), 2.);
        if (rz.y == 1.)
        {
            float mx = abs(pos.x*.1)-10.;
            mx = smoothstep(-20.,10.,mx);
            col = mix(vec3(0.,0.37,0),vec3(0.2,.17,0.15),mx);
        }
        else
            col = sin(vec3(0.35,0.4,0.48)*rz.y*PALETTE)*0.5+.55;
        col = col*dif + col*0.4 + .3*fre*col;
    }
    
    //lasers
    vec3 enp =enpos();
    enp.z += time*50.;
    vec3 rn = enp - ro;
    float tgt = dot(eye, normalize(rn));
    if (tgt > .997)
    {
        vec3 ray1 = vec3(0.7, 1., -1);
        vec3 ray2 = vec3(-0.7, 1., -1);
        ray1.xy *= ori; ray2.xy *= ori;
        float lz = segm(ro,rd,ro-ray1,up*0.5+ro+(eye-ray1*0.01)*30.);
        lz += segm(ro,rd,ro-ray2,up*.5+ro+(eye-ray2*0.01)*30.);
        float sw = mod(floor(time*20.),2.);
        lz *= sw;
        col = col*(1.-smoothstep(0.0,1.,lz))+lz*vec3(1.,0.,0.);
        //hit (cant really have explosions since I don't have a function for hit times)
        if (tgt > .999)
        {
            vec2 d = hash2(time);
            rd.xy += d*0.03;
            rn.xy += d*10.;
            float s = sw*smoothstep(0.9998, .9999,dot(rd,normalize(rn)));
            col = col*(1.-smoothstep(0., 1., s))+s*vec3(1.-d.x, .0, 0.1);
        }
    }
    
    //hud
    float lk = 0.;
    if (tgt > .99)lk = 4.;
    vec3 hud = makeHud(p,tgt);
    col = col*(1.-smoothstep(0., 1., hud.y+hud.x+hud.z))+hud;   
    //scanlines
    col *= (sin(p.y*1.3*iResolution.x)*0.15)*(sin(p.y*10.+time*410.)*0.4)+1.;
    
	fragColor = vec4( col, 1.0 );
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
