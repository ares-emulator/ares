#version 450
// Created by Reinder Nijhoff 2014
// @reindernijhoff
//
// https://www.shadertoy.com/view/Xtf3zn
//
// car model is made by Eiffie
// shader 'Shiny Toy': https://www.shadertoy.com/view/ldsGWB

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

#define BUMPMAP
#define MARCHSTEPS 128
#define MARCHSTEPSREFLECTION 48
#define LIGHTINTENSITY 5.

//----------------------------------------------------------------------

const vec3 backgroundColor = vec3(0.2,0.4,0.6) * 0.09;
#define time (iGlobalTime + 90.)

//----------------------------------------------------------------------
// noises

float hash( float n ) {
    return fract(sin(n)*687.3123);
}

float noise( in vec2 x ) {
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*157.0;
    return mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
               mix( hash(n+157.0), hash(n+158.0),f.x),f.y);
}

const mat2 m2 = mat2( 0.80, -0.60, 0.60, 0.80 );

float fbm( vec2 p ) {
    float f = 0.0;
    f += 0.5000*noise( p ); p = m2*p*2.02;
    f += 0.2500*noise( p ); p = m2*p*2.03;
    f += 0.1250*noise( p ); p = m2*p*2.01;
//    f += 0.0625*noise( p );
    
    return f/0.9375;
}

//----------------------------------------------------------------------
// distance primitives

float udRoundBox( vec3 p, vec3 b, float r ) {
  return length(max(abs(p)-b,0.0))-r;
}

float sdBox( in vec3 p, in vec3 b ) {
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdSphere( in vec3 p, in float s ) {
    return length(p)-s;
}

float sdCylinder( in vec3 p, in vec2 h ) {
    vec2 d = abs(vec2(length(p.xz),p.y)) - h;
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

//----------------------------------------------------------------------
// distance operators

float opU( float d2, float d1 ) { return min( d1,d2); }
float opS( float d2, float d1 ) { return max(-d1,d2); }
float smin( float a, float b, float k ) { return -log(exp(-k*a)+exp(-k*b))/k; } //from iq

//----------------------------------------------------------------------
// Map functions

// car model is made by Eiffie
// shader 'Shiny Toy': https://www.shadertoy.com/view/ldsGWB

float mapCar(in vec3 p0){ 
	vec3 p=p0+vec3(0.0,1.24,0.0);
	float r=length(p.yz);
	float d= length(max(vec3(abs(p.x)-0.35,r-1.92,-p.y+1.4),0.0))-0.05;
	d=max(d,p.z-1.0);
	p=p0+vec3(0.0,-0.22,0.39);
	p.xz=abs(p.xz)-vec2(0.5300,0.9600);p.x=abs(p.x);
	r=length(p.yz);
	d=smin(d,length(max(vec3(p.x-0.08,r-0.25,-p.y-0.08),0.0))-0.04,8.0);
	d=max(d,-max(p.x-0.165,r-0.24));
	float d2=length(vec2(max(p.x-0.13,0.0),r-0.2))-0.02;
	d=min(d,d2);

	return d;
}

float dL; // minimal distance to light

float map( const in vec3 p ) {
	vec3 pd = p;
    float d;
    
    pd.x = abs( pd.x );
    pd.z *= -sign( p.x );
    
    float ch = hash( floor( (pd.z+18.*time)/40. ) );
    float lh = hash( floor( pd.z/13. ) );
    
    vec3 pdm = vec3( pd.x, pd.y, mod( pd.z, 10.) - 5. );
    dL = sdSphere( vec3(pdm.x-8.1,pdm.y-4.5,pdm.z), 0.1 );
    
    dL = opU( dL, sdBox( vec3(pdm.x-12., pdm.y-9.5-lh,  mod( pd.z, 91.) - 45.5 ), vec3(0.2,4.5, 0.2) ) );
    dL = opU( dL, sdBox( vec3(pdm.x-12., pdm.y-11.5+lh, mod( pd.z, 31.) - 15.5 ), vec3(0.22,5.5, 0.2) ) );
    dL = opU( dL, sdBox( vec3(pdm.x-12., pdm.y-8.5-lh,  mod( pd.z, 41.) - 20.5 ), vec3(0.24,3.5, 0.2) ) );
   
    if( lh > 0.5 ) {
	    dL = opU( dL, sdBox( vec3(pdm.x-12.5,pdm.y-2.75-lh,  mod( pd.z, 13.) - 6.5 ), vec3(0.1,0.25, 3.2) ) );
    }
    
    vec3 pm = vec3( mod( pd.x + floor( pd.z * 4. )*0.25, 0.5 ) - 0.25, pd.y, mod( pd.z, 0.25 ) - 0.125 );
	d = udRoundBox( pm, vec3( 0.245,0.1, 0.12 ), 0.005 ); 
    
    d = opS( d, -(p.x+8.) );
    d = opU( d, pd.y );

    vec3 pdc = vec3( pd.x, pd.y, mod( pd.z+18.*time, 40.) - 20. );
    
    // car
    if( ch > 0.75 ) {
        pdc.x += (ch-0.75)*4.;
	    dL = opU( dL, sdSphere( vec3( abs(pdc.x-5.)-1.05, pdc.y-0.55, pdc.z ),    0.025 ) );
	    dL = opU( dL, sdSphere( vec3( abs(pdc.x-5.)-1.2,  pdc.y-0.65,  pdc.z+6.05 ), 0.025 ) );

        d = opU( d,  mapCar( (pdc-vec3(5.,-0.025,-2.3))*0.45 ) );
 	}
    
    d = opU( d, 13.-pd.x );
    d = opU( d, sdCylinder( vec3(pdm.x-8.5, pdm.y, pdm.z), vec2(0.075,4.5)) );
    d = opU( d, dL );
    
	return d;
}

//----------------------------------------------------------------------

vec3 calcNormalSimple( in vec3 pos ) {   
    const vec2 e = vec2(1.0,-1.0)*0.005;

    vec3 n = normalize( e.xyy*map( pos + e.xyy ) + 
					    e.yyx*map( pos + e.yyx )   + 
					    e.yxy*map( pos + e.yxy )   + 
					    e.xxx*map( pos + e.xxx )   );  
    return n;
}

vec3 calcNormal( in vec3 pos ) {
    vec3 n = calcNormalSimple( pos );
    if( pos.y > 0.12 ) return n;

#ifdef BUMPMAP
    vec2 oc = floor( vec2(pos.x+floor( pos.z * 4. )*0.25, pos.z) * vec2( 2., 4. ) );

    if( abs(pos.x)<8. ) {
		oc = pos.xz;
    }
    
     vec3 p = pos * 250.;
   	 vec3 xn = 0.05*vec3(noise(p.xz)-0.5,0.,noise(p.zx)-0.5);
     xn += 0.1*vec3(fbm(oc.xy)-0.5,0.,fbm(oc.yx)-0.5);
    
    n = normalize( xn + n );
#endif
    
    return n;
}

vec3 integer1, integer2, nor1;
vec4 lint1, lint2;

float intersect( in vec3 ro, in vec3 rd ) {
	const float precis = 0.001;
    float h = precis*2.0;
    float t = 0.;
    integer1 = integer2 = vec3( -500. );
    lint1 = lint2 = vec4( -500. );
    float mld = 100.;
    
	for( int i=0; i < MARCHSTEPS; i++ ) {
        h = map( ro+rd*t );
		if(dL < mld){
			mld=dL;
            lint1.xyz = ro+rd*t;
			lint1.w = abs(dL);
		}
        if( h < precis ) {
            integer1.xyz = ro+rd*t;
            break;
        } 
        t += max(h, precis*2.);
    }
    
    if( integer1.z < -400. || t > 300.) {
        // check intersection with plane y = -0.1;
        float d = -(ro.y + 0.1)/rd.y;
		if( d > 0. ) {
			integer1.xyz = ro+rd*d;
	    } else {
        	return -1.;
    	}
    }
    
    ro = ro + rd*t;
    nor1 = calcNormal(ro);
    ro += 0.01*nor1;
    rd = reflect( rd, nor1 );
    t = 0.0;
    h = precis*2.0;
    mld = 100.;
    
    for( int i=0; i < MARCHSTEPSREFLECTION; i++ ) {
        h = map( ro+rd*t );
		if(dL < mld){
			mld=dL;            
            lint2.xyz = ro+rd*t;
			lint2.w = abs(dL);
		}
        if( h < precis ) {
   			integer2.xyz = ro+rd*t;
            return 1.;
        }   
        t += max(h, precis*2.);
    }

    return 0.;
}

//----------------------------------------------------------------------
// shade

vec3 shade( in vec3 ro, in vec3 pos, in vec3 nor ) {
    vec3  col = vec3(0.5);
    
    if( abs(pos.x) > 15. || abs(pos.x) < 8. ) col = vec3( 0.02 );
    if( pos.y < 0.01 ) {
        if( abs( integer1.x ) < 0.1 ) col = vec3( 0.9 );
        if( abs( abs( integer1.x )-7.4 ) < 0.1 ) col = vec3( 0.9 );
    }    
    
    float sh = clamp( dot( nor, normalize( vec3( -0.3, 0.3, -0.5 ) ) ), 0., 1.);
  	col *= (sh * backgroundColor);  
 
    if( abs( pos.x ) > 12.9 && pos.y > 9.) { // windows
        float ha = hash(  133.1234*floor( pos.y / 3. ) + floor( (pos.z) / 3. ) );
        if( ha > 0.95) {
            col = ( (ha-0.95)*10.) * vec3( 1., 0.7, 0.4 );
        }
    }
    
	col = mix(  backgroundColor, col, exp( min(max(0.1*pos.y,0.25)-0.065*distance(pos, ro),0.) ) );
  
    return col;
}

vec3 getLightColor( in vec3 pos ) {
    vec3 lcol = vec3( 1., .7, .5 );
    
	vec3 pd = pos;
    pd.x = abs( pd.x );
    pd.z *= -sign( pos.x );
    
    float ch = hash( floor( (pd.z+18.*time)/40. ) );
    vec3 pdc = vec3( pd.x, pd.y, mod( pd.z+18.*time, 40.) - 20. );

    if( ch > 0.75 ) { // car
        pdc.x += (ch-0.75)*4.;
        if(  sdSphere( vec3( abs(pdc.x-5.)-1.05, pdc.y-0.55, pdc.z ), 0.25) < 2. ) {
            lcol = vec3( 1., 0.05, 0.01 );
        }
    }
    if( pd.y > 2. && abs(pd.x) > 10. && pd.y < 5. ) {
        float fl = floor( pd.z/13. );
        lcol = 0.4*lcol+0.5*vec3( hash( .1562+fl ), hash( .423134+fl ), 0. );
    }
    if(  abs(pd.x) > 10. && pd.y > 5. ) {
        float fl = floor( pd.z/2. );
        lcol = 0.5*lcol+0.5*vec3( hash( .1562+fl ),  hash( .923134+fl ), hash( .423134+fl ) );
    }
   
    return lcol;
}

float randomStart(vec2 co){return 0.8+0.2*hash(dot(co,vec2(123.42,117.853))*412.453);}

//----------------------------------------------------------------------
// main

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {    
    vec2 q = fragCoord.xy / iResolution.xy;
	vec2 p = -1.0 + 2.0*q;
	p.x *= iResolution.x / iResolution.y;
        
    if (q.y < .12 || q.y >= .88) {
		fragColor=vec4(0.,0.,0.,1.);
		return;
	}
    
    // camera
    float z = time;
    float x = -10.9+1.*sin(time*0.2);
	vec3 ro = vec3(x,  1.3+.3*cos(time*0.26), z-1.);
    vec3 ta = vec3(-8.,1.3+.4*cos(time*0.26), z+4.+cos(time*0.04));
    
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
	vec3 rd = normalize( -p.x*uu + p.y*vv + 2.2*ww );
    
    vec3 col = backgroundColor;

    // raymarch
    float ints = intersect(ro+randomStart(p)*rd ,rd );
    if(  ints > -0.5 ) {
        
        // calculate reflectance
		float r = 0.09;     	        
        if( integer1.y > 0.129 ) r = 0.025 * hash(  133.1234*floor( integer1.y / 3. ) + floor( integer1.z / 3. ) );
        if( abs(integer1.x) < 8. ) {
            if( integer1.y < 0.01 ) { // road
	            r = 0.007*fbm(integer1.xz);
            } else { // car
                r = 0.02;
            }
        }
        if( abs( integer1.x ) < 0.1 ) r *= 4.;
        if( abs( abs( integer1.x )-7.4 ) < 0.1 ) r *= 4.;
        
        r *= 2.;
        
        col = shade( ro, integer1.xyz, nor1 );
        
        if( ints > 0.5 ) {
            col += r * shade( integer1.xyz, integer2.xyz, calcNormalSimple(integer2.xyz) );
        }  
        if( lint2.w > 0. ) {            
            col += (r*LIGHTINTENSITY*exp(-lint2.w*7.0)) * getLightColor(lint2.xyz);
        } 
    } 
      
    // Rain (by Dave Hoskins)
	vec2 st = 256. * ( p* vec2(.5, .01)+vec2(time*.13-q.y*.6, time*.13) );
    float f = noise( st ) * noise( st*0.773) * 1.55;
	f = 0.25+ clamp(pow(abs(f), 13.0) * 13.0, 0.0, q.y*.14);
    
    if( lint1.w > 0. ) {
        col += (f*LIGHTINTENSITY*exp(-lint1.w*7.0)) * getLightColor(lint1.xyz);
    }  
    
	col += 0.25*f*(0.2+backgroundColor);

    // post processing
	col = pow( clamp(col,0.0,1.0), vec3(0.4545) );
	col *= 1.2*vec3(1.,0.99,0.95);   
	col = clamp(1.06*col-0.03, 0., 1.);  
    q.y = (q.y-.12)*(1./0.76);
	col *= 0.5 + 0.5*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1 ); 

    fragColor = vec4( col, 1.0 );
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
