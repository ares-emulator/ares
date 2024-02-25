#version 450
// a 'famous' shader by Kali
// ported by mudlord 2016
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
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = Position.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

float orgy(vec2 p) {
    float time = float(global.FrameCount)*0.015;
    float pl=1., expsmo=0.;
    float t=sin(time*20.);
    float a=-.35+t*.02;
    p*=mat2(cos(a),sin(a),-sin(a),cos(a));
    p=p*.07+vec2(.718,-.565)+t*.017+vec2(0.,t*.014);
    for (int i=0; i<13; i++) {
        p.x=abs(p.x);
        p=p*2.+vec2(-2.,.85)-t*.04;
        p/=min(dot(p,p),1.03);  
        float l=length(p*p);
        expsmo+=exp(-1.2/abs(l-pl));
        pl=l;
    }
    return expsmo;
}


void main( void )
{
    vec2 uv = (vTexCoord.xy*global.OutputSize.xy) /global.OutputSize.xy-.5;
    uv.y = -uv.y;
    uv.x*=global.OutputSize.x/global.OutputSize.y;
    vec2 p=uv; p.x*=1.2; p*=2.1;
    p.y+=.2; p.x += .1;
    float o=clamp(orgy(p)*.07,.13,1.); o=pow(o,1.8);
    vec3 col=vec3(o*.8,o*o*.87,o*o*o*.9);
    float hole=length(uv+vec2(.09,0.252))-.45;
    FragColor = vec4(col*2.1+.1, 1.0 );
}
