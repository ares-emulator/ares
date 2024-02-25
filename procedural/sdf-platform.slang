#version 450
// Xafer - 2017-04-30
// https://www.shadertoy.com/view/4dlyzl

// Reflections, refractions, lights such

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

#define NEAR 0.01
#define FAR 12.0

#define STEP_SIZE 0.1
#define MAX_ITER 300
#define MAX_CYCLES 2
#define DELTA 0.01
#define A_SPEED 0.3
#define CAM_DIST 3.3

#define RECAST_MAX 5

#define GLOW_POWER 6.0
#define GLOW_COLOR vec3(0.2,0.4,0.4)
#define GRASS_SIZE vec2(0.005,0.4)

#define SKY_COLOR vec3(0.1,0.3,0.4)


//Structures

struct Light
{
    vec3 pos;
    vec3 color;
    float radius;
    float intensity;
};
    
struct lComp
{
	vec3 specular;
    float specularIntensity;
    vec3 diffuse;
};

struct Ray
{
    vec3 pos;
    vec3 dir;
    float dist;
    float near;
};
    
struct Material
{
    vec3 color;
    float specular;
    float diffuse;//
    float ambient;//Ambient light factor
    float refl;//How reflective is a surface
    float refr;//Refraction index
    float opacity;//For refractions
};

    
//Signed Distance Functions    

float sdSphere(vec3 p)
{
    return length(p) - 1.0;
}

float sdPlane(vec3 p)
{
    return -p.y;
}

float sdBox( vec3 p)
{
  vec3 d = abs(p) - vec3(1.0);
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float opAdd(float d1, float d2)
{
    return min(d1,d2);
}

float opSub(float d1, float d2)
{
    return max(-d1,d2);
}

float opInter(float d1, float d2)
{
    return max(d1,d2);
}


//Combinations
    
float walls(vec3 p)
{
    p.y -= 1.0;
    p.y * 2.0;
    
    vec3 p2 = p;
    vec3 p3 = p;
    
    float width = 0.1;
    vec2 dist = vec2(5.0);
    
    p2.xz = mod(p2.xz,dist)-(dist/2.0);
    p2.x /= width;
    p2.z /= 2.0;
    
    p3.xz = mod(p3.xz,dist)-(dist/2.0);
    p3.z /= width;
    p3.x /= 2.0;
    
    float env = opAdd(sdPlane(p),opAdd(sdBox(p2),sdBox(p3)));
    
    env = opSub(max(-p.y - 0.3, p.y + 0.2),max(env,sdSphere(p/(CAM_DIST-0.4))));
    
    env = opAdd(env,sdSphere(p/0.7 + vec3(0,0.5,0)));
    
    return opSub(sdSphere(p),env);
}

float map(vec3 pos, float factor)
{
    return walls(pos);
}

float map(vec3 pos)
{
 	return map(pos,0.0);
}

//Environment propreties

vec3 getNormal(vec3 p)
{
	vec2 delta = vec2 (DELTA, 0.0);
	vec3 normal = normalize (vec3 (
		map (p + delta.xyy) - map (p - delta.xyy),
		map (p + delta.yxy) - map (p - delta.yxy),
		map (p + delta.yyx) - map (p - delta.yyx)));
	
	return normalize(normal);
}



Material getMaterial(vec3 pos)
{
    Material mat;
    mat.color = vec3(sin(pos.y),cos(pos.x),sin(pos.z))/2.0 + 0.5;
    mat.refl = 0.0;
    mat.refr = 0.8;
    mat.opacity = 0.7;
    
    mat.color = mix(mat.color,getNormal(pos),0.2);
    
    return mat;
}

//RayMarching

Ray march(Ray ray, int iterations)
{
    float n = 1.0 / min(iResolution.x, iResolution.y);
    
    for(int i = 0; i < iterations; i++)
    {
    	ray.near = map(ray.pos,sqrt(ray.dist)*n);
        
        if(ray.near < DELTA || ray.dist > FAR)
            break;
        
        float n = ray.near * STEP_SIZE;
        ray.pos += ray.dir * n;
        ray.dist += n;
    }
    
    return ray;
}

lComp getLighting(Ray ray)
{
    lComp lightComposition;
    
    //Looping through lights
    Light light;
    light.radius = 3.5;
    light.intensity = 1.0;
    light.pos = vec3(0,-1,0);
    light.color = vec3(1.0,0.8,0.6);
    
    float specular;
    float diffuse;
    
    vec3 rayPos = ray.pos;
    vec3 dir = light.pos - ray.pos;
    float lDist = length(dir);
    
    dir = normalize(dir);
    
    rayPos += DELTA*dir;
    
    float near = 0.0;
    float dist = 0.0;
    
    float l;
    
    for(int i =0 ; i < MAX_ITER;i++)
    {
        near = map(rayPos);
        l = length(rayPos-light.pos);
        if(near < DELTA || dist > lDist)
            break;
        
        float n = abs(near * 0.1);
        
        rayPos += n*dir;
        dist += n;
    }
    
    if(near < DELTA)
        diffuse = 0.0;
    else if(dist > lDist)
    {
        diffuse = max(0.0,1.0 - (lDist/light.radius));
        specular = dot(reflect(ray.dir,getNormal(ray.pos)),dir)/2.0 + 0.5;
        specular = -cos(specular*3.1416)/2.0 + 0.5;
        specular = pow(specular,lDist*lDist);
        //if(specular > 0.9);
        //	specular /= 20.0;
    }
    
    vec3 normal = getNormal(ray.pos);
        
    lightComposition.diffuse = mix(SKY_COLOR, light.color, diffuse);
    lightComposition.specular = light.color;
    lightComposition.specularIntensity = specular;
    
    return lightComposition;
}

//Marching through refractions

vec3 getRefraction(Ray ray)
{
    vec3 color;
    float colorFactor;
    
    float intensity = 1.0;
   	Material mat = getMaterial(ray.pos);;
    
    for(int i = 0; i < MAX_CYCLES; i++)
    {
        vec3 normal = getNormal(ray.pos);
        ray.dir = refract(ray.dir, normal, mat.refr);
        
        vec3 m = ray.dir*DELTA*2.0;
        
        int inside = 0;
        
        for(int j = 0; j < MAX_ITER && (ray.near < DELTA || inside == 1); j++)
        {
            ray.pos += m;
            ray.dist += DELTA*2.0;
            ray.near = map(ray.pos);
        }
        
        ray.dir = refract(ray.dir,normal,mat.refr);
        ray = march(ray,MAX_ITER);
        
        if(ray.near > DELTA || ray.dist > FAR)
        {
            color += SKY_COLOR*intensity;
            colorFactor += intensity;
            break;
        }
        
        lComp lightComposition = getLighting(ray);
        
        mat = getMaterial(ray.pos);
        
        vec3 col = mix(mat.color * lightComposition.diffuse,lightComposition.specular, lightComposition.specularIntensity);
        
        color += mix(col,SKY_COLOR,ray.dist/FAR)*intensity;
        colorFactor += intensity;
        
        intensity *= 1.0 - mat.opacity;
    }
    
    return color/colorFactor;
}

//Marching through reflections

vec3 getReflection(Ray ray)
{
    vec3 color;
    float colorFactor;
    
    float intensity = 1.0;
    
    for(int i = 0; i < MAX_CYCLES; i++)
    {
        vec3 normal = getNormal(ray.pos);
        ray.dir = reflect(ray.dir,normal);
        ray.pos += ray.dir * DELTA;
        
        ray = march(ray, MAX_ITER);
        
        Material mat = getMaterial(ray.pos);
        
        intensity *= mat.refl;
        
        lComp lightComposition = getLighting(ray);
        
        vec3 col = mix(mat.color * lightComposition.diffuse, lightComposition.specular, lightComposition.specularIntensity);
        
        color += mix(col,SKY_COLOR,ray.dist/FAR)*intensity;
        colorFactor += intensity;
    }
    
    return color/colorFactor;
}

//Getting the color at the specified point

vec3 getColor(Ray ray)
{
    
    vec3 color;
    vec3 normal = getNormal(ray.pos);
    
    if(ray.dist > FAR)
    {
		color = SKY_COLOR;
    }
    else if(ray.near <= DELTA)
    {
        Material mat = getMaterial(ray.pos);
        
        color = mat.color;
        
        lComp lightComposition = getLighting(ray);
       	color *= lightComposition.diffuse;
        
        if(mat.refr > 0.0)
            color = mix(getRefraction(ray),color,mat.opacity);
        
    	if(mat.refl > 0.0)
            color = mix(color,getReflection(ray),mat.refl);
        
        color = mix(color, lightComposition.specular, lightComposition.specularIntensity);
    
    	color = mix(color, SKY_COLOR , ray.dist/FAR);
        color = mix(color, SKY_COLOR , dot(ray.dir,normal)/2.0 + 0.5);
    }
    
	return color;
}

vec3 castRay(vec3 origin, vec3 direction)
{
    //Generating ray
    Ray ray;
    ray.pos = origin;
    ray.dir = direction;
    ray.dist = 0.0;
    
    //Move the ray to a surface up to far distance
    ray = march(ray, MAX_ITER*2);
    
    return getColor(ray);
}

//Initialisation

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    //Setting up screen-correct uv
	vec2 uv = ( fragCoord.xy / iResolution.xy ) * 2.0 - vec2( 1 );
    uv.x *= iResolution.x/iResolution.y;
    
    //Setting up rotation
    float sa = sin( iGlobalTime * A_SPEED );
    float ca = cos( iGlobalTime * A_SPEED );
    
    float cDist = CAM_DIST + sin(iGlobalTime * A_SPEED);
    
    //Creating ray
    vec3 or = vec3(sa*cDist,0.5,-ca*cDist);
    vec3 di = -normalize(vec3(uv.x,uv.y,-1.0));
    
    //Rotating orientation
    mat3 r;
    r[0] = vec3(ca,0,sa);
    r[1] = vec3(0,1,0);
    r[2] = vec3(-sa,0,ca);
    di = r*di;
    
    vec3 color = castRay(or,di);
    
	fragColor = vec4(color,1);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
