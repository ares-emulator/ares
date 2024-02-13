#version 450
// Skyline 3D -  FabriceNeyret2 - 2015-07-02
// https://www.shadertoy.com/view/4tlSWr

// in the spirit of the 2D skylines (e.g., https://www.shadertoy.com/view/4tlXzM )... but in 3D :-)  ( I even reproduced the original temporal aliasing :-D  )

// Be sure to wait long enough ;-)

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

#define T iGlobalTime

// --- using the base ray-marcher of Trisomie21: https://www.shadertoy.com/view/4tfGRB#

vec4 bg = vec4(0); // vec4(0,0,.3,0); 

void mainImage( out vec4 f, vec2 w ) {
    vec4 p = vec4(w,0,1)/iResolution.yyxy-.5, d,c; p.x-=.4; // init ray
    // r(p.xz,.13); r(p.yz,.2); r(p.xy,.1);   // camera rotations
    d =p;                                 // ray dir = ray0-vec3(0)
    p.x += 15.*T; p += 2.*d;
    f = bg;
    float l,x=1e9, closest = 999.;
   
    for (float i=1.; i>0.; i-=.01)  {
       
        vec4 u = floor(p/8.), t = mod(p, 8.)-4., ta; // objects id + local frame
      	u.y = 0.; 
        u = sin(78.17*(u+u.yzxw));                     // randomize ids
        
        c = p/p*1.2;
        ta = abs(t);
        x=1e9; 
        if (sin(17.*(u.x+u.y+u.z))>.95) { // 10% of blocks
            ta.y = p.y + 30.*u.x - .3*pow(abs(.03*floor(p.z)),3.) + 35.;
            x = max(ta.x,max(ta.y,ta.z))  -3.; 
         }
        closest = min(closest, p.y+150.); 
        
        // artifacts: passed a object, we might be fooled about dist to next (hidden in next modulo-tile)
#if 1        // if dist to box border is closest to object, go there.  <<< the working solution ! (at mod8 scale)
        vec4 k, k1 = (4.-t)/d ,k2 = (-4.-t)/d, dd; 
        k = min (k1-1e5*sign(k1),k2-1e5*sign(k2))+1e5; // ugly trick to get the min only if positive.
        // 2 less ugly/costly formulations, but less robust close to /0 :
        //   k = mix(k1,k2, .5+.5*sign(k2));
        //   dd = d+.001*clamp(1.-d*d,.999,1.); k = (4.*sign(dd)-t)/dd;
        l = min(k.x,min(k.y,k.z));
        if (l<x) { p+= 1.*d*(l+0.01); continue; }
#endif
        // if (x<.01) c = texture(iChannel0,.1*(p.xy+p.yz));
      
        if(x<.01) // hit !
            { f = mix(bg,c,i*i); break;  }  // color texture + black fog
       
        p += d*x;       // march ray
     }
    //if (length(f)==0.) f = vec4(1,1,.6,0)*smoothstep(.31,.3,length(w/iResolution.y-vec2(1.3,.7)));
    f += vec4(1) * exp(-.01*closest)*(.5+.5*cos(1.+T/8.)); // thanks kuvkar ! 
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
