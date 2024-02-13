#version 450
#
//Playing with inversion and volumetric light.
//fizer 2015-06-06

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

void mainImage(out vec4 c, vec2 q)
{
    float a=iGlobalTime*.1+1.,b=.5,g,e,t=0.,s;
    vec3 r=vec3(0.,0.,3.),w=normalize(vec3((q-iResolution.xy/2.)/iResolution.y,-.5)),p;

    mat2 x=mat2(cos(a),sin(a),sin(a),-cos(a)),y=mat2(cos(b),sin(b),sin(b),-cos(b));

    w.xz=y*w.xz;
    r.xz=y*r.xz;

    w.yz=x*w.yz;
    r.yz=x*r.yz;

    c.rgb=vec3(0.,0.,.02);

    for(int i=0;i<150;++i)
    {
        p=r+w*t;

        float f=.25,d=1e4;
        for(int j=0;j<2;++j)
        {
            s=.2*dot(p,p);
            p=p/s;
            f*=s;
            g=p.z;
            e=atan(p.y,p.x);
            p=(mod(p,2.)-1.)*1.25;
        }

        d=min((length(abs(p.xy)-1.3)-.1)*f,1e2);

        if(d<1e-3)
            break;

        c.rgb+=vec3(.3,.4,.8)*(pow(.5+.5*cos(g*.5+a*77.+cos(e*10.)),16.))*
            (1.-smoothstep(0.,1.,70.*d))*.25;

        t+=d;
    }
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
  FragCoord.y = -FragCoord.y;
  mainImage(FragColor,FragCoord);
}
