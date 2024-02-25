#version 450
// Cave Quest -  Bergi - 2016-06-25
// https://www.shadertoy.com/view/XdGXD3

// Lot's of finetuning to get a nice cave fly-through. 
// Greetings to Kali and Shane

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

/** Cave Quest 
	https://www.shadertoy.com/view/XdGXD3

	(cc) 2016, stefan berke

	Based on "Kali Trace" https://www.shadertoy.com/view/4sKXWG

	Interesting things here might be:
	- the distance formula in itself
	- wrapping 3d-space using sin() for endless kali-set patterns
	- kali-set used for normals, micro-normals and texturing
	- epsilon in normal estimation for artistic tuning

*/


// minimum distance to axis-aligned planes in kali-space
// uses eiffie's mod (/p.w) https://www.shadertoy.com/view/XtlGRj
// to keep the result close to a true distance function
vec3 kali_set(in vec3 pos, in vec3 param)
{
    vec4 p = vec4(pos, 1.);
    vec3 d = vec3(100.);
    for (int i=0; i<9; ++i)
    {
        p = abs(p) / dot(p.xyz,p.xyz);
        d = min(d, p.xyz/p.w);
        p.xyz = p.zxy - param;
    }
    return d;
}

// average of all iterations in kali-set
vec3 kali_set_av(in vec3 p, in vec3 param)
{
    vec3 d = vec3(0.);
    for (int i=0; i<13; ++i)
    {
        p = abs(p) / dot(p,p);
        d += exp(-p*8.);
        p.xyz = p.zxy - param;
    }
    return d / 8.;
}

// endless texture  
vec3 kali_tex(in vec3 p, in vec3 par)
{
    vec3 k = kali_set_av(sin(p*3.)*.3, par);
    return 3.*k;
}

// endless texture normal
vec3 kali_tex_norm(in vec3 p, in vec3 param, vec3 mask, float eps)
{
    vec2 e = vec2(eps, 0.);
    return normalize(vec3(
        dot(kali_tex(p+e.xyy, param),mask) - dot(kali_tex(p-e.xyy, param),mask),
        dot(kali_tex(p+e.yxy, param),mask) - dot(kali_tex(p-e.yxy, param),mask),
        dot(kali_tex(p+e.yyx, param),mask) - dot(kali_tex(p-e.yyx, param),mask)));
}



// camera path
vec3 path(in float z)
{
    float t = z;
    vec3 p = vec3(sin(t)*.5, 
                  .26*sin(t*3.16), 
                  z);
    return p;
}


float DE(in vec3 p, in vec3 param)
{
    // tube around path
    float r = .13+.1*sin(p.z*.89);
    vec3 pp = p - path(p.z); float d = r-max(abs(pp.x), abs(pp.y));
    
    // displacement
    vec3 k = kali_set(sin(p), param);
    d += k.x+k.y+k.z;
    //d += max(k.x,max(k.y,k.z));
    //d += min(k.x,min(k.y,k.z));
    return d;
}

vec3 DE_norm(in vec3 p, in vec3 param, in float eps)
{
    vec2 e = vec2(eps, 0.);
    return normalize(vec3(
        DE(p+e.xyy, param) - DE(p-e.xyy, param),
        DE(p+e.yxy, param) - DE(p-e.yxy, param),
        DE(p+e.yyx, param) - DE(p-e.yyx, param)));
}


// lighting/shading currently depends on this beeing 1.
const float max_t = 1.;

// common sphere tracing
// note the check against abs(d) to get closer to surface
// in case of overstepping
float trace(in vec3 ro, in vec3 rd, in vec3 param)
{
    float t = 0.001, d = max_t;
    for (int i=0; i<50; ++i)
    {
        vec3 p = ro + rd * t;
        d = DE(p, param);
        if (abs(d) <= 0.00001 || t >= max_t)
            break;
        t += d * .5; // above kali-distance still needs a lot of fudging
    }
    return t;
}

// "Enhanced Sphere Tracing"
// Benjamin Keinert(1) Henry Schäfer(1) Johann Korndörfer Urs Ganse(2) Marc Stamminger(1)
// 1 University of Erlangen-Nuremberg, 2 University of Helsinki
// 
// It was a try... disabled by default (see rayColor() below)
// Just here for experimentation
// Obviously the algorithm does not like "fudging" which is needed for my distance field..
// It renders more stuff close to edges but creates a lot of artifacts elsewhere
float trace_enhanced(in vec3 ro, in vec3 rd, in vec3 param)
{
    float omega = 1.2; // overstepping
    float t = 0.001;
    float candidate_error = 100000.;
    float candidate_t = t;
    float previousRadius = 0.;
    float stepLength = .0;
    float signedRadius;
    float pixelRadius = .012;
    float fudge = 0.6;
    for (int i = 0; i < 50; ++i) 
    {
        signedRadius = DE(rd*t + ro, param);
        float radius = abs(signedRadius);
        bool sorFail = omega > 1. && (radius + previousRadius) < stepLength;
        if (sorFail) 
        {
        	stepLength -= omega * stepLength;
        	omega = 1.;
        } 
        else 
        {
        	stepLength = signedRadius * omega;
        }
        previousRadius = radius;
        float error = radius / t;
        if (!sorFail && error < candidate_error) 
        {
        	candidate_t = t;
        	candidate_error = error;
    	}
    	if (!sorFail && error < pixelRadius || t > max_t)
    		break;
    	t += stepLength * fudge;
    }
    return (t > max_t || candidate_error > pixelRadius)
        ? max_t : candidate_t;
}

// common ambient occlusion
float traceAO(in vec3 ro, in vec3 rd, in vec3 param)
{
    float a = 0., t = 0.01;
    for (int i=0; i<5; ++i)
    {
        float d = DE(ro+t*rd, param);
       	a += d / t;
        t += abs(d);
    }
    return clamp(a / 8., 0., 1.);
}

// environment map, also drawn from kaliset
vec3 skyColor(in vec3 rd)
{
    //vec3 par = vec3(0.075, 0.565, .03);
    vec3 par = vec3(.9, .81, .71);
    
    vec3 c = kali_set(sin(rd*6.), par);
    c = pow(min(vec3(1.), c*2.+vec3(1.,.86,.6)), 1.+114.*c);
    
    return clamp(c, 0., 1.);
}


// trace and color
vec3 rayColor(in vec3 ro, in vec3 rd)
{
    // magic params for kali-set
    vec3 par1 = vec3(.9, .6+.5*sin(ro.z/50.), 1.),	// scene geometry 
         par2 = vec3(.63, .55, .73),				// normal/bump map
         par3 = vec3(1.02, 0.82, 0.77); 			// normal/texture
    
#if 1
    float t = trace(ro, rd, par1);
#else    
    float t = trace_enhanced(ro, rd, par1);
#endif    
    vec3 p = ro + t * rd;
    float d = DE(p, par1);
    
    vec3 col = vec3(0.);

    // did ray hit?
    if (d < 0.03) 
    {
        float scr_eps = max(0.001, (t-0.1)*0.025);
        // "some" texture values
        vec3 kt = kali_tex(p, par3);
        // surface normal
        vec3 n = DE_norm(p, par1, 0.5*scr_eps), nn = n;
        // normal displacement
        n = normalize(n + 0.3*kali_tex_norm(p, par3+0.1*n, vec3(1), scr_eps));
        n = normalize(n + 0.3*DE_norm(sin(n*3.+kt), par2, 2.*scr_eps)); // micro-bumps
        // reflected ray
        vec3 rrd = reflect(rd,n);
		// normal towards light
        vec3 ln = normalize(path(p.z+.1) - p);
		// 1. - occlusion
        float ao = pow(traceAO(p, n, par1), 1.+3.*t);
        // surface color
        vec3 col1 = .45 * (vec3(.7,1.,.4) + kali_tex(p, par3));
        vec3 col2 = vec3(1.,.8,.6) + .3 * vec3(1.,.7,-.6) * kali_tex(p, par3);
        vec3 k = kali_set_av(sin(p*(1.+3.*ao))*.3, par3);
        vec3 surf = (.1 + .9 * ao) 
            		//* vec3(1.);
            		* mix(col1, col2, min(1., pow(ao*2.2-.8*kt.x,5.)));
		// desaturate
        surf += .24 * (dot(surf,vec3(.3,.6,.1)) - surf);

        // -- lighting --
        
        float fres = pow(max(0., 1.-dot(rrd, n)), 1.) / (1.+2.*t);

        // phong
        surf += .25 * ao * max(0., dot(n, ln));
        // spec
        float d = max(0., dot(rrd, ln));
        surf += .4 * pow(ao*1.2,5.) * (.5 * d + .7 * pow(d, 8.));

        // fresnel highlight
        surf += clamp((t-.06)*8., 0.,1.6) * 
            	(.2+.8*ao) * vec3(.7,.8,1.) * fres;
        
        // environment map
        surf += .2 * (1.-fres) * ao * skyColor(rrd);
    
        // distance fog
    	col = surf * pow(1.-t / max_t, 1.3);
    }
    
    return col;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 suv = fragCoord.xy / iResolution.xy;
	vec2 uv = (fragCoord.xy - iResolution.xy*.5) / iResolution.y * 2.;
        
    float ti = (iGlobalTime-14.)/8.;
    
    vec3 ro = path(ti);
    vec3 look = path(ti+.5);
    float turn = (ro.x-look.x)*1.; 
        
    // lazily copied from Shane
    // (except the hacky turn param)
    float FOV = .7; // FOV - Field of view.
    vec3 fwd = normalize(look-ro);
    vec3 rgt = normalize(vec3(fwd.z, turn, -fwd.x));
    vec3 up = cross(fwd, rgt);
    
    vec3 rd = normalize(fwd + FOV*(uv.x*rgt + uv.y*up));
    
    
    vec3 col = rayColor(ro, rd);
    //col = skyColor(rd);
    
    col *= pow(1.-dot(suv-.5,suv-.5)/.5, .6);
    
	fragColor = vec4(pow(col,vec3(.8)),1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
