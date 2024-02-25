#version 450
layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
} global;

#pragma stage vertex
layout(location = 0) in  vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
const vec2 madd = vec2(0.5, 0.5);

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = gl_Position.xy;
}

#pragma stage fragment
layout(location = 0) in  vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

float time      = float(global.FrameCount)*0.0325;
vec2 resolution = global.OutputSize.xy;
float fade      = 0.45;

const float PI  = 3.14159265358979323846;

float speed     = time*0.25;
float ground_x  = 0.125 - 0.25*cos(PI*speed*0.25);
float ground_y  = 0.125 + 0.25*sin(PI*speed*0.25);
float ground_z  = speed*0.125;

vec2 rotate(vec2 k, float t)
{
    return vec2(cos(t)*k.x - sin(t)*k.y, sin(t)*k.x + cos(t)*k.y);
}

float scene(vec3 p)
{
    float ball_p = 0.25;
    float ball_w = ball_p*1.0;
    float ball   = length(mod(p.xyz, ball_p) - ball_p*0.5) - ball_w;
    float hole_w = ball_p*0.55;
    float hole   = length(mod(p.xyz, ball_p) - ball_p*0.5) - hole_w;
    float pipe_p = 0.015;
    float pipe_w = pipe_p*0.42; //-0.00375*sync;
    float pipe_y = length(max(abs(mod(p.xy, pipe_p) - pipe_p*0.5) - pipe_w,0.0));
    float pipe_z = length(max(abs(mod(p.xz, pipe_p) - pipe_p*0.5) - pipe_w,0.0));

    return max(max(ball, -hole), max(pipe_y, max(pipe_y, pipe_z)));
}

vec3 getNormal(vec3 pos) 
{ 
    vec3 e = vec3(0.0, 0.0001, 0.0); 

    return normalize(vec3(scene(pos + e.yxx) - scene(pos - e.yxx),
                          scene(pos + e.xyx) - scene(pos - e.xyx),
                          scene(pos + e.xxy) - scene(pos - e.xxy))); 
}

float render_scene(vec3 ray_origin, vec3 ray_dir, float t)
{
    const int ray_n = 96;
    for (int i = 0; i < ray_n; i++)
    {
        float k = scene(ray_origin + ray_dir*t);
        t      += k*fade;
    }
    return t;   
}

void main(void)
{
    vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
    FragCoord.y    = -FragCoord.y;

    vec2 position = (FragCoord.xy/resolution.xy);
    vec2 p        = -1.0 + 2.0*position;
    
    //set up camera
    float speed = time*0.5;
    vec3 dir = normalize(vec3(p*vec2(1.0, 1.0), 1.));
    dir.yz   = rotate(dir.yz,PI*1.0*sin(speed*0.25));     // rotation x
    dir.zx   = rotate(dir.zx,PI*1.0*cos(speed*0.25));     // rotation y
    dir.xy   = rotate(dir.xy, -speed*0.5);                 // rotation z
    vec3 ray = vec3(ground_x, ground_y, ground_z);
    
    //the raymarch
    float t  = 0.0;
    t        = render_scene(ray, dir, t);
    vec3 hit = ray + dir*t;
    t       += hit.x;
    
    //get normal for reflection
    vec3 n = getNormal(hit);
    
    //render reflection
    dir        = reflect(dir, n);
    dir        = normalize(refract(dir, n, .82));   
    t          = render_scene(ray, dir, t); 
    float c    = (n.x*1.0 + n.y + n.z)*0.5;   
    vec3 color = vec3(0., c*t*0.125*p.x + t*0.1, c*t*0.);
    color     *= 2.412;
    
    FragColor = vec4(color, 1.0);
}
