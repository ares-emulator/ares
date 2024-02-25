#version 450
//kirby jump
// fizzer 2018-01-31
// https://www.shadertoy.com/view/lt2fD3

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
   float ZOOM;
   float X_AXIS;
   float Y_AXIS;
   float SPEED;
} global;

#pragma parameter ZOOM "Zoom" 2.0 0.001 10.0 0.1
#pragma parameter X_AXIS "Pan Horizontal" 0.0 -1.0 1.0 0.01
#pragma parameter Y_AXIS "Pan Vertical" 0.0 -1.01 1.0 0.01
#pragma parameter SPEED "Animation Speed" 1.0 0.0 5.0 0.1

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
const vec2 madd = vec2(0.5, 0.5);
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = gl_Position.xy;
   vTexCoord = vTexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
float iGlobalTime = float(global.FrameCount)*0.025 * global.SPEED;
vec2 iResolution = global.OutputSize.xy;

// polynomial smooth min (from IQ)
float smin( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}


float smax(float a,float b, float k)
{
    return -smin(-a,-b,k);
}

mat2 rotmat(float a)
{
    return mat2(cos(a),sin(a),-sin(a),cos(a));
}

float shoesDist(vec3 p)
{
    vec3 op=p;
    float d=1e4;

    p.y-=1.5;

    // right shoe
    op=p;
    p-=vec3(-.5,-.6,-.9);
    p.yz=rotmat(-.7)*p.yz;
    p.xz=rotmat(0.1)*p.xz;
    d=min(d,-smin(p.y,-(length(p*vec3(1.6,1,1))-.64),.2));
    p=op;

    // left shoe
    op=p;
    p-=vec3(.55,-.8,0.4);
    p.x=-p.x;
    p.yz=rotmat(1.4)*p.yz;
    d=min(d,-smin(p.y,-(length(p*vec3(1.6,1,1))-.73),.2));
    p=op;
    return d;
}

float sceneDist(vec3 p)
{
    vec3 op=p;
    float d=shoesDist(p);

    d=min(d,p.y);
    p.y-=1.5;

    // torso
    d=min(d,length(p)-1.);


    // left arm
    op=p;
    p-=vec3(.66,.7,0);
    p.xz=rotmat(-0.1)*p.xz;
    d=smin(d,(length(p*vec3(1.8,1,1))-.58),.07);
    p=op;

    // right arm
    op=p;
    p-=vec3(-.75,0.2,0);
    d=smin(d,(length(p*vec3(1,1.5,1))-.54),.03);
    p=op;

    // mouth
    p.y-=.11;
    float md=smax(p.z+.84,smax(smax(p.x-.2,p.y-.075,.2),dot(p,vec3(.7071,-.7071,0))-.1,.08),.04);
    p.x=-p.x;
    md=smax(md,smax(p.z+.84,smax(smax(p.x-.2,p.y-.075,.2),dot(p,vec3(.7071,-.7071,0))-.1,.08),.01),.13);
    d=smax(d,-md,.012);

    // tongue
    p=op;
    d=smin(d,length((p-vec3(0,.03,-.75))*vec3(1,1,1))-.16,.01);

    return min(d,10.);
}



vec3 sceneNorm(vec3 p)
{
    vec3 e=vec3(1e-3,0,0);
    float d = sceneDist(p);
    return normalize(vec3(sceneDist(p + e.xyy) - sceneDist(p - e.xyy), sceneDist(p + e.yxy) - sceneDist(p - e.yxy),
                          sceneDist(p + e.yyx) - sceneDist(p - e.yyx)));
}


// from simon green and others
float ambientOcclusion(vec3 p, vec3 n)
{
    const int steps = 4;
    const float delta = 0.15;

    float a = 0.0;
    float weight = 4.;
    for(int i=1; i<=steps; i++) {
        float d = (float(i) / float(steps)) * delta; 
        a += weight*(d - sceneDist(p + n*d));
        weight *= 0.5;
    }
    return clamp(1.0 - a, 0.0, 1.0);
}

// a re-shaped cosine, to make the peaks more pointy
float cos2(float x){return cos(x-sin(x)/3.);}

float starShape(vec2 p)
{
    float a=atan(p.y,p.x)+iGlobalTime/3.;
    float l=pow(length(p),.8);
    float star=1.-smoothstep(0.,(3.-cos2(a*5.*2.))*.02,l-.5+cos2(a*5.)*.1);
    return star;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    float an=cos(iGlobalTime)*.1;

    vec2 ot=uv*2.-1.;
    ot.y*=iResolution.y/iResolution.x;
    vec3 ro=vec3(0.,1.4,4.);
    vec3 rd=normalize(vec3(ot.xy,-1.3));

    rd.xz=mat2(cos(an),sin(an),sin(an),-cos(an))*rd.xz;
    ro.xz=mat2(cos(an),sin(an),sin(an),-cos(an))*ro.xz;

    float s=20.;

    // primary ray
    float t=0.,d=0.;
    for(int i=0;i<80;++i)
    {
        d=sceneDist(ro+rd*t);
        if(d<1e-4)
            break;
        if(t>10.)
            break;
        t+=d*.9;
    }

    t=min(t,10.0);

    // shadow ray
    vec3 rp=ro+rd*t;
    vec3 n=sceneNorm(rp);
    float st=5e-3;
    vec3 ld=normalize(vec3(2,4,-4));
    for(int i=0;i<20;++i)
    {
        d=sceneDist(rp+ld*st);
        if(d<1e-5)
            break;
        if(st>5.)
            break;
        st+=d*2.;
    }

    // ambient occlusion and shadowing
    vec3 ao=vec3(ambientOcclusion(rp, n));
    float shad=mix(.85,1.,step(5.,st));

    ao*=mix(.3,1.,.5+.5*n.y);

    // soft floor shadow
    if(rp.y<1e-3)
        ao*=mix(mix(vec3(1,.5,.7),vec3(1),.4)*.6,vec3(1),smoothstep(0.,1.6,length(rp.xz)));



    vec3 diff=vec3(1);
    vec3 emit=vec3(0);

    // skin
    diff*=vec3(1.15,.3,.41)*1.4;
    diff+=.4*mix(1.,0.,smoothstep(0.,1.,length(rp.xy-vec2(0.,1.9))));
    diff+=.5*mix(1.,0.,smoothstep(0.,.5,length(rp.xy-vec2(.7,2.5))));
    diff+=.36*mix(1.,0.,smoothstep(0.,.5,length(rp.xy-vec2(-1.1,1.8))));

    if(rp.y<1e-3)
        diff=vec3(.6,1,.6);

    // mouth
    diff*=mix(vec3(1,.3,.2),vec3(1),smoothstep(.97,.99,length(rp-vec3(0,1.5,0))));

    // shoes
    diff=mix(vec3(1.,.05,.1),diff,smoothstep(0.,0.01,shoesDist(rp)));
    diff+=.2*mix(1.,0.,smoothstep(0.,.2,length(rp.xy-vec2(-0.5,1.4))));
    diff+=.12*mix(1.,0.,smoothstep(0.,.25,length(rp.xy-vec2(0.57,.3))));

    // bounce light from the floor
    diff+=vec3(.25,1.,.25)*smoothstep(-.3,1.7,-rp.y+1.)*max(0.,-n.y)*.7;

    vec3 orp=rp;
    rp.y-=1.5;
    rp.x=abs(rp.x);

    // blushes
    diff*=mix(vec3(1,.5,.5),vec3(1),smoothstep(.1,.15,length((rp.xy-vec2(.4,.2))*vec2(1.,1.65))));

    rp.xy-=vec2(.16,.45);
    rp.xy*=.9;
    orp=rp;
    rp.y=pow(abs(rp.y),1.4)*sign(rp.y);

    // eye outline
    diff*=smoothstep(.058,.067,length((rp.xy)*vec2(.9,.52)));

    rp=orp;
    rp.y+=.08;
    rp.y-=pow(abs(rp.x),2.)*16.;

    // eye reflections
    emit+=vec3(.1,.5,1.)*(1.-smoothstep(.03,.036,length((rp.xy)*vec2(.7,.3))))*max(0.,-rp.y)*10.;

    rp=orp;
    rp.y-=.12;

    // eye highlights
    emit+=vec3(1)*(1.-smoothstep(.03,.04,length((rp.xy)*vec2(1.,.48))));

    // fresnel
    diff+=pow(clamp(1.-dot(-rd,n),0.,.9),4.)*.5;

    // background and floor fade
    vec3 backg=vec3(1.15,.3,.41)*.9;
    ot.x+=.6+iGlobalTime/50.;
    ot.y+=cos(floor(ot.x*2.)*3.)*.1+.2;
    ot.x=mod(ot.x,.5)-.25;
    backg=mix(backg,vec3(1.,1.,.5),.1*starShape((ot-vec2(0.,.6))*8.)*smoothstep(9.,10.,t));
    diff=mix(diff,backg,smoothstep(.9,10.,t));

    fragColor.rgb=mix(vec3(.15,0,0),vec3(1.,1.,1.),ao)*shad*diff*1.1;
    fragColor.rgb+=emit;
    fragColor.a = 1.0;

    fragColor.rgb=pow(fragColor.rgb,vec3(1./2.4));
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = ((vTexCoord / global.ZOOM) + 0.5 + vec2(global.X_AXIS, global.Y_AXIS)) * global.OutputSize.xy;
  mainImage(FragColor,FragCoord);
}
