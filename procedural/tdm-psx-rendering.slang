#version 450
// PSX Rendering -  TDM - 2016-08-06
// https://www.shadertoy.com/view/Mt3Gz2

// Lack of perspective-correct texturing, z-buffer, float data type and bilinear filtering lead to this kind of buggy rendering.

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

/*
"PSX rendering" by Alexander Alekseev aka TDM - 2016
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
*/

#define PSX_MODE

#ifdef PSX_MODE
	#define INT_VERTICES
	vec2 RESOLUTION = vec2(320.0, 240.0);
#else
	#define BILINEAR
	vec2 RESOLUTION = iResolution.xy;
#endif

// math
float _cross(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}
vec3 barycentric(vec2 a, vec2 b, vec2 c, vec2 p) {    
    vec2 ab = b - a, ac = c - a, ap = p - a;
    vec2 vw = vec2(_cross(ap,ac),_cross(ab,ap)) / _cross(ab,ac);
    return vec3(1.0-vw.x-vw.y, vw);
}
float quantization(float a, float b) {
    return floor(a * b) / b;
}
vec2 quantization(vec2 a, float b) {
    return floor(a * b) / b;
}
vec2 quantization(vec2 a, vec2 b) {
    return floor(a * b) / b;
}
vec3 quantization(vec3 a, float b) {
    return floor(a * b) / b;
}
float hash( vec2 p ) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}

float noise1(vec2 p) {
    #ifndef BILINEAR
		return hash(floor(p));
    #else    
        vec2 i = floor(p);
        vec2 f = fract(p);
    	vec2 tx = mix(vec2(hash(i),hash(i+vec2(0.,1.))) ,
                      vec2(hash(i+vec2(1.,0.)),hash(i+vec2(1.))),f.x);
        return mix(tx.x,tx.y,f.y);
    #endif
}
mat4 getRotMatrix(vec3 a) {
    vec3 s = sin(a);
    vec3 c = cos(a);    
    mat4 ret;
    ret[0] = vec4(c.y*c.z,c.y*s.z,-s.y,0.0);
    ret[1] = vec4(s.x*s.y*c.z-c.x*s.z,s.x*s.y*s.z+c.x*c.z,s.x*c.y,0.0);
    ret[2] = vec4(c.x*s.y*c.z+s.x*s.z, c.x*s.y*s.z-s.x*c.z,   c.x*c.y,0.0);    
    ret[3] = vec4(0.0,0.0,0.0,1.0);
    return ret;
}
mat4 getPosMatrix(vec3 p) {   
    mat4 ret;
    ret[0] = vec4(1.0,0.0,0.0,p.x);
    ret[1] = vec4(0.0,1.0,0.0,p.y);
    ret[2] = vec4(0.0,0.0,1.0,p.z);   
    ret[3] = vec4(0.0,0.0,0.0,1.0);
    return ret;
}

// textures
vec4 textureGround(vec2 uv) {
    const vec2 RES = vec2(8.0, 8.0);    
    vec2 NEW       = uv * RES;
    float n = noise1(NEW);
    n = n * 0.2 + 0.5;
    return vec4(n*0.9,n*0.6,n*0.4,1.0);
}

vec4 textureWall(vec2 uv) {
    const vec2 RES = vec2(32.0, 16.0);
    vec2 NEW       = uv * RES;
    vec2 iuv = floor(NEW);    
    float n = noise1(NEW);
    n = n * 0.5 + 0.25;
    float nc = n * (smoothstep(1.0,0.4, iuv.x / RES.x) * 0.5 + 0.5);    
    return vec4(nc * 0.4, nc * 1.0, nc * 0.5, n + uv.x - abs(uv.y-0.5) );
}

vec4 textureBox(vec2 uv) {
    const vec2 RES = vec2(8.0, 8.0);
    vec2 NEW       = uv * RES;
    vec2 iuv = (floor(NEW) + 0.5) / RES;  
    float n = noise1(NEW);
    n = max(abs(iuv.x - 0.5), abs(iuv.y - 0.5)) * 2.0;
    n = n * n;
    n = 0.5 + n * 0.4 + noise1(NEW) * 0.1;
    return vec4(n, n*0.8, n*0.5, 1.0);
}

vec4 textureSky(vec2 uv) {
    const vec2 RES = vec2(8.0, 32.0);    
    vec2 NEW = uv * RES;
    float n = noise1(NEW);
    n = n * 0.05 + 0.8;
    return vec4(0.5,n*1.0,n*1.1,1.0);
}   

// rasterization
void triangle(vec2 p,
         vec2 v0, vec2 v1, vec2 v2,
         vec2 uv0, vec2 uv1, vec2 uv2,
         in int tex, inout vec3 color) {
    
    if(_cross(v2-v0,v1-v0) <= 1e-4) return;
    
    vec3 bary = abs(barycentric(v0,v1,v2, p));
    if(bary.x + bary.y + bary.z <= 1.0001) {
        vec2 uv = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z; 
        vec4 frag;        
        if(tex == 0) {
    		frag = textureGround(uv);
        } else if(tex == 1) {
            frag = textureWall(uv);
        } else {
            frag = textureBox(uv);
        }
        if(frag.w > 0.5) color = frag.xyz;
    }
}
void quad(vec2 p,
         vec2 v0, vec2 v1, vec2 v2, vec2 v3,
         vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3,
        in int tex,  inout vec3 color) {    
    triangle(p, v0,v1,v2, uv0,uv1,uv2, tex,color);
    triangle(p, v2,v3,v0, uv2,uv3,uv0, tex,color);
}

// geometry transformation engine
void gte(inout vec3 v, mat4 mat) {   
    
    // perspective
    v = (vec4(v,1.0) * mat).xyz;
    v.xy /= max(v.z, 1.0);
    
    v *= 2.0;
    
    // quantization to simulate int
    #ifdef INT_VERTICES    	
		const vec2 QUANT = vec2(320.0, 240.0) * 0.25;
        v.xy = quantization(v.xy, QUANT);
    #endif    
}

// renderer
void gpu(vec2 p,
         vec2 v0, vec2 v1, vec2 v2, vec2 v3,
         vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3,
         in int tex, inout vec3 color) {
    
    quad(p,
         v0,v1,v2,v3,
         uv0,uv1,uv2,uv3,
         tex,color);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord.xy / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;    
    
    uv = quantization(uv, RESOLUTION * 0.5);
        
    const float WIDTH = 1.3;
    const float HEIGHT = 6.0;
    const float DEPTH = 4.0;  
    const float LOD = 4.0;
    
    float time = iGlobalTime * 2.;    
    
    vec3 posv = vec3(-WIDTH*3.0,-2.0, -time+5.0);
    vec3 rotv = vec3(sin(time)*0.05 + 0.1,
             sin(time*0.9)*0.05,
             sin(time*0.7)*0.05);
    
    // int-like position
    #ifdef INT_VERTICES    	
        posv = quantization(posv, 64.0);
        rotv = quantization(rotv, 256.0);    	
    #endif
    
    mat4 cam = getPosMatrix(posv);    
    mat4 rot = getRotMatrix(rotv); 
    cam = cam * rot;
        
    vec3 c = textureSky(uv + vec2(rotv.y,-rotv.x) * 3.0).xyz;
    
    // ground
    float z_offset = -floor((posv.z + DEPTH * 1.5) / DEPTH) * DEPTH * 0.5;
    float poly_depth = DEPTH;
    
    for(int dip = 0; dip < 32; dip++) {
        
        // kinda LOD
        z_offset += step(mod(float(dip),4.0), 0.5) * poly_depth * 0.5;
        #ifdef PSX_MODE
        if(dip > 11) poly_depth = DEPTH * LOD; 
        #endif
        
        vec3 vert[4];
        vert[0] = vec3(-WIDTH,0.0, poly_depth); 
        vert[1] = vec3(-WIDTH,0.0, 0.0);   
        vert[2] = vec3( WIDTH,0.0, 0.0);
        vert[3] = vec3( WIDTH,0.0, poly_depth);
   
        vec3 posv = vec3(mod(float(dip),4.0) * WIDTH,
                         0.0,
                         z_offset);        

        mat4 pos = getPosMatrix(posv * 2.0);
        mat4 mat = pos * cam;

        for(int i = 0; i < 4; i++) gte(vert[i], mat);             
        
        gpu(uv,
            vert[3].xy,vert[2].xy,vert[1].xy,vert[0].xy,
            vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
            0, c);
    }
    
    // walls
    z_offset = -floor((posv.z + DEPTH ) / DEPTH) * DEPTH * 0.5;
    poly_depth = DEPTH;
    
    for(int dip = 0; dip < 8; dip++) {
        
        // kinda LOD
        z_offset += poly_depth * 0.5;
        #ifdef PSX_MODE
        if(dip > 2) poly_depth = DEPTH * LOD;     
        #endif
        
        vec3 vert[4];
        vert[0] = vec3(0.0,HEIGHT, poly_depth); 
        vert[1] = vec3(0.0,HEIGHT, 0.0);   
        vert[2] = vec3(0.0,0.0, 0.0);
        vert[3] = vec3(0.0,0.0, poly_depth);
   
        vec3 posv = vec3(WIDTH * 3.5,
                         0.0,
                         z_offset);
        //posv.z -= z_offset;

        mat4 posm = getPosMatrix(posv * 2.0);
        mat4 mat = posm * cam;

        for(int i = 0; i < 4; i++) gte(vert[i], mat);             
        
        gpu(uv,
            vert[0].xy,vert[1].xy,vert[2].xy,vert[3].xy,
            vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
            1, c);
        
        
        vert[0] = vec3(0.0,HEIGHT, poly_depth); 
        vert[1] = vec3(0.0,HEIGHT, 0.0);   
        vert[2] = vec3( 0.0,0.0, 0.0);
        vert[3] = vec3( 0.0,0.0, poly_depth);
        
        posv = vec3(-WIDTH * 0.5,
                         0.0,
                         z_offset);

        posm = getPosMatrix(posv * 2.0);
        mat = posm * cam;

        for(int i = 0; i < 4; i++) gte(vert[i], mat);  
        
        gpu(uv,
            vert[3].xy,vert[2].xy,vert[1].xy,vert[0].xy,
            vec2(1.0,0.0), vec2(1.0), vec2(0.0,1.0),vec2(0.0),
            1, c);
    }
    
    // box
    vec3 vert[8];
    vert[0] = vec3(-1.0,-1.0, 1.0);
    vert[1] = vec3(-1.0, 1.0, 1.0);    
    vert[2] = vec3( 1.0, 1.0, 1.0);    
    vert[3] = vec3( 1.0,-1.0, 1.0);
    vert[4] = vec3(-1.0,-1.0,-1.0);
    vert[5] = vec3(-1.0, 1.0,-1.0);    
    vert[6] = vec3( 1.0, 1.0,-1.0);    
    vert[7] = vec3( 1.0,-1.0,-1.0);

    vec3 box_posv = vec3(-posv.x,
                     2.0,
                     -posv.z + 15.0);

    rotv = vec3(time * 0.5, time * 0.6, time * 0.7);    
    mat4 posm = getRotMatrix(rotv) * getPosMatrix(box_posv);
    mat4 mat = posm * cam;

    for(int i = 0; i < 8; i++) {
        vert[i].y *= 0.65;
        gte(vert[i], mat);
    }

    gpu(uv,
        vert[3].xy,vert[2].xy,vert[1].xy,vert[0].xy,
        vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
        2, c);
    gpu(uv,
        vert[4].xy,vert[5].xy,vert[6].xy,vert[7].xy,
        vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
        2, c);
    
    gpu(uv,
        vert[7].xy,vert[6].xy,vert[2].xy,vert[3].xy,
        vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
        2, c);
    gpu(uv,
        vert[0].xy,vert[1].xy,vert[5].xy,vert[4].xy,
        vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
        2, c);
    
    gpu(uv,
        vert[2].xy,vert[6].xy,vert[5].xy,vert[1].xy,
        vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
        2, c);    
    gpu(uv,
        vert[0].xy,vert[4].xy,vert[7].xy,vert[3].xy,
        vec2(0.0),vec2(0.0,1.0),vec2(1.0),vec2(1.0,0.0),
        2, c);
    
    // fragment
	fragColor = vec4(c,1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
