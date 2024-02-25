#version 450
// Ray * Bert -  2013-03-08
// https://www.shadertoy.com/view/4sl3RH

//					  --------
// Ray*Bert!!!       ( @!#?@! ) 
//					  --| |---
//						|/
// By       Dave Hoskins
// Finished for now... may come back later to put the other characters in.
// V. 0.8  - Added colour changes and a fixed path for Q*Bert.
// V. 0.7  - Better fur and ambient lighting.
// V. 0.64 - Moving Q*bert.Behaves differently on different machines! :(
// V. 0.63 - Pupil watching.
// V. 0.62 - Asynchronous bounces.
// V. 0.61 - Fur.
// V. 0.60 - In the middle of getting old Q to move about.


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

#define REFLECTIONS_ON
							
#define PI  3.1415926535

vec3 areaPlane = normalize(vec3(-1.0, 1.0, 1.0));
vec3 lightDir = normalize(vec3(-1437.0, 1743.0, 430.0));

vec3 QbertPos;
float QbertRotation;
vec3 balls[3];
float squish[4];
float radius[3];

const vec3 ballStart1 = vec3(-.0, 2.6, -1.);
const vec3 ballStart2 = vec3( 1.0, 2.6, 0.);


const vec3 addLeft  = vec3(-1.0, -1.0, 0.0);
const vec3 addRight = vec3(.0, -1.0, 1.0);

const vec3  QbertStart = vec3(-3.0, -1.3, .00);


float time;

float rand( float n )
{
    return fract(sin(n*1233.23)*43758.5453);
}

float hash( float n )
{
    return fract(sin(n)*43758.5453123);
}

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);

    float n = p.x + p.y*57.0 + 113.0*p.z;

    float res = mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                        mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
                    mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                        mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
    return res;
}


vec2 rotate2D(vec2 p, float a)
{
    float sa = sin(a);
    float ca = cos(a);
    vec2 r;
    r.x = ca*p.x + sa*p.y;
    r.y = -sa*p.x + ca*p.y;
    return r;
}

// Finding the movement location at a set time. Loopy IF-tastic!!...
void GetLocation(int i, float s, out vec3 outPos1, out vec3 outPos2)
{
	int end = int(mod(s, 8.0))-1;
	float r = floor(s/8.0) + float(i*547);
	vec3 pos;
	if (rand(float(r)) > .5)
	{
		pos = ballStart1;
	}else
	{
		pos = ballStart2;
	}

	for (int t = 0; t < 9; t++)
	{
		if (t == 0)
		{
			outPos1 = pos + vec3(0.0, 3.5, 0.0);
		}
		if (t == end)
		{
			outPos1 = pos;
		}
		if (t == end+1)
		{
			outPos2 = pos;
			if (t == 7)
			{
				outPos2 = outPos1 + vec3(0.0, -8.5, 0.0);
			}
		}

		if (rand(float(t)+r) > .5)
		{
			pos += addLeft;
		}else
		{
			pos += addRight;
		}
	}
}

//--------------------------------------------------------------------------------------------
vec3 MoveQbert(int n, in vec3 pos, out float dir)
{
	if (n == 0)
	{
		pos -= addLeft;
		dir = -90.0;
	}else
	if (n == 1)
	{
		pos -= addRight;
		dir = 180.0;
	}else
	if (n == 3)
	{
		pos += addLeft;
		dir = 90.0;
	}else
	{
		pos += addRight;
		dir = 0.0;
	}
	return pos;
}


//--------------------------------------------------------------------------------------------
int DirTable[19];
float GetQbertLocation(float s, out vec3 out1, out vec3 out2)
{
	int end = int(mod(s, 18.0));
	float r = floor(s/18.0);
	vec3 pos = QbertStart;
	float dir = 0.0;
	float outDir;
	vec3 newPos;

	for (int t = 0; t < 19; t++)
	{
		if (t == end)
		{
			out1 = pos;
		}
		if (t == end+1)
		{
			out2 = pos;
			outDir = dir / 180.0 * PI;
		}
		int val = DirTable[t];
		pos = MoveQbert(val, pos, dir);
	}
	return outDir;

}

//----------------------- Distance Estimation fields -------------------
float deTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xy)-t.x,p.z);
  return length(q)-t.y;
}

float deQBertEyes(vec3 p)
{
	float t = squish[3];//clamp(fra*.25, 0.0, 0.2)-.2;
	p.y -= t;
	p.xz = rotate2D(p.xz, QbertRotation);
	vec3 pos = p + vec3(-.08, -.17, -.18);
	
    float d = length(pos)-.12;
	pos = p + vec3(.08, -.17, -.18);
    d = min(d, length(pos)-.12);
	return d;	
}

float deQBert(vec3 pos)
{
	pos.xz = rotate2D(pos.xz, QbertRotation);
	vec3 p = pos;
	
	// Torso...
	float t = squish[3];//clamp(fra*.25, 0.0, 0.2)-.2;
	p.y -=t;
	p.y *= .9;
	float d = length(p)-.32;
	p = pos * vec3(1.0, 1.0, .3);
	p.z -= .1;
	p.y -= t;
	d = min(d, deTorus(p, vec2(.07, .06)));

	// Vertical legs...
	p = (pos * vec3(1.0, .4, 1.0))- vec3(-.13, -.2, -.1);
	d = min(d, length(p)-.06);
	p = (pos * vec3(1.0, .4, 1.0))- vec3(.13, -.2, -.1);
	d = min(d, length(p)-.06);
	// Feet...
	p = (pos * vec3(.4, .8, .3))- vec3(-.05, -.5, .01);
	d = min(d, length(p)-.03);
	p = (pos * vec3(.4, .8, .3))- vec3(.05, -.5, .01);
	d = min(d, length(p)-.03);
	return d;	
}

float deBall(vec3 p, float s)
{
    return length(p)-s;
}

float dePlane(vec3 p, vec3 planeN)
{
    return dot(p, planeN);
}

float PlayArea(vec3 p)
{	
    float d;
    d = dePlane(p, areaPlane);
    return d;
}

//--------------------------------------------------------------------------------------------
float Scene(in vec3 p, out int which)
{	
    float d = 1000.0, f;
	
	for (int i =0; i < 3; i++)
	{
		vec3 pos = p-balls[i];

		// Squish it...
		pos.xz /= squish[i];
		pos.y *= squish[i];
		f = deBall(pos, radius[i]);
		if (f < d)
		{
			d = f;
			which = i;
		}
	}
	
	f = deQBert(p - QbertPos);
	if (f < d)
	{
		d = f;
		which = 4;
	}

	f = deQBertEyes(p - QbertPos);
	if (f < d)
	{
		d = f;
		which = 5;
	}

    return d;
}

//------------------------------ Lighting ------------------------------------
// Calculate scene normal
vec3 SceneNormal(vec3 pos )
{
    float eps = 0.001;
    vec3 n;
	int m;
    float d = Scene(pos, m);
    n.x = Scene( vec3(pos.x+eps, pos.y, pos.z), m ) - d;
    n.y = Scene( vec3(pos.x, pos.y+eps, pos.z),m ) - d;
    n.z = Scene( vec3(pos.x, pos.y, pos.z+eps),m ) - d;
    return normalize(n);
}

vec3 HueGradient(float t)
{
	t += .4;
    vec3 p = abs(fract(t + vec3(1.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
	return (clamp(p - 1.0, 0.0, 1.0));
}
// Colouring in the fragments...
vec3 LightCubes(vec3 pos, vec3 n)
{
	vec3 qpos = QbertStart;
	float dir;
	ivec3 ipos = ivec3(floor(pos+.5));
	float foundHit = 0.0;
	int end = int(mod(time*.8, 18.0));
	
	vec3 areaColour = vec3(.53, .7, .85);
	for (int t = 0; t < 18; t++)
	{
		qpos = MoveQbert(DirTable[t], qpos, dir);
		ivec3 ip = ivec3(floor(qpos+.5));
		if (ip == ipos && n.y >= .8)
		{
			if (t >= end) 
			foundHit = .5;	
			else
			foundHit = 1.5;
		}
	}
	if (foundHit > 0.0) areaColour = HueGradient((floor((time*.8/18.0))+foundHit)/4.23);
	
    float diff = dot(n, lightDir);
    diff = max(diff, 0.0);
    return diff * areaColour;	
}

vec3 LightCharacters(vec3 pos, vec3 norm, vec3 dir, int m)
{
	float specular = 0.0;
	vec3 ballColour;
	float specSize = 8.0;
	vec3 specColour = vec3(1.0, 1.0, 1.0);
	if (m == 7)
	{
		ballColour = vec3(1.0, 1.0, 1.0);
		specColour = vec3(0.0, 0.0, 0.0);
		specSize = 2.0;
	}
	
	if (m == 6)
	{
		norm += (noise((pos-QbertPos)*42.0)*.5)-.5;
		ballColour = vec3(1.2, 0.42, 0.);
	}
	else 
	{
		vec3 reflect = ((-2.0*(dot(dir, norm))*norm)+dir);
		specular = pow(max(dot(reflect, lightDir), 0.0), specSize);
		
		if (m == 2)
		{
			ballColour = vec3(1.0, 0.0, 1.0);
		}else
		if (m == 3)
		{
			ballColour = vec3(1.0, 0.0, 0.0);
		}else
		if (m == 4)
		{
			ballColour = vec3(0.0, 1.0, 0.0);
		}
	}

	float diff = dot(norm, lightDir);
    diff = max(diff, 0.3);

	return mix(diff * ballColour, specColour, specular);	
}

//--------------------------------------------------------------------------------------------
float DistanceFields(vec3 ro, vec3 rd, out int hit, out vec3 pos)
{
	float len = .0;
	float d;
	hit = 0;
	int m = 0;
	for (int st = 0; st < 22; st++)
	{
		pos = ro + rd * len;
		d = Scene(pos, m);
		if (d < 0.01)
		{
			hit = m+1;
			break;
		}
			len += d;
	}
	return len;
}
 
//--------------------------------------------------------------------------------------------
// Voxel grid search that I found in 1994 in Graphics Gems IV - "Voxel Traversal along a 3D Line"!
// This (Amanatides & Woo) varient is from another shader on here, but with some calculations removed,
// and the distance value fixed.
// I had to use voxels as standard distance fields don't work for perfectly aligned cubes.

float VoxelTrace(vec3 ro, vec3 rd, out bool hit, out vec3 hitNormal, out vec3 pos)
{
    const int maxSteps = 41;
    vec3 voxel = floor(ro)+.5001;
    vec3 step = sign(rd);
	//voxel = voxel - vec3(rd.x > 0.0, rd.y > 0.0, rd.z > 0.0);
    vec3 tMax = (voxel - ro) / rd;
    vec3 tDelta = 1.0 / abs(rd);
    vec3 hitVoxel = voxel;
    
	
    hit = false;
	
    float hitT = 0.0;
    for(int i=0; i < maxSteps; i++)
	{
		if (!hit)
		{
			float d = PlayArea(voxel);        
			if (d <= 0.0 && !hit)
			{
				hit = true;
				hitVoxel = voxel;
				break;
			}
			bool c1 = tMax.x < tMax.y;
			bool c2 = tMax.x < tMax.z;
			bool c3 = tMax.y < tMax.z;
			if (c1 && c2) 
			{ 
				voxel.x += step.x;
				tMax.x += tDelta.x;
				if (!hit) 
				{
					hitNormal = vec3(-step.x, 0.0, 0.0);
					hitT = tMax.x-tDelta.x;
                    
				}
			} else if (c3 && !c1) 
			{
				voxel.y += step.y;
				tMax.y += tDelta.y;
				if (!hit) 
				{
					hitNormal = vec3(0.0, -step.y, 0.0);	
					hitT = tMax.y-tDelta.y;
				}
			} else
			{
				voxel.z += step.z;
				tMax.z += tDelta.z;
				if (!hit) 
				{
					hitNormal = vec3(0.0, 0.0, -step.z);		
					hitT = tMax.z-tDelta.z;
				}
			}
		}
    }
	
	if (hit)
	{
		if (hitVoxel.x > 1.75 || hitVoxel.z < -1.75 || hitVoxel.y < -3.5)
		{
			hit = false;
			hitT = 1000.0;
		}
	}
	pos = ro + hitT * rd;
	return hitT;
}

//--------------------------------------------------------------------------------------------
// Do all the ray casting for voxels and normal distance fields...
float TraceEverything(vec3 ro,vec3 rd, out int material, out vec3 hitNormal, out vec3 pos)
{
	bool hit1;
	int hit2;
	vec3 pos2;
    float dist = VoxelTrace(ro, rd, hit1, hitNormal, pos);
	float dist2 = DistanceFields(ro, rd, hit2, pos2);
	if (hit2 > 0 && dist2 < dist)
	{
		hitNormal = SceneNormal(pos2);
		pos = pos2;
		material = hit2+1;
	}else
	if (hit1)
	{
		material = 1;
	}else
	{
		material = 0;
	}
	return dist;
}

//--------------------------------------------------------------------------------------------
int TraceShadow(vec3 ro, vec3 rd)
{
	int hit;
	vec3 pos;
	float dist2 = DistanceFields(ro, rd, hit, pos);
	return hit;
}

//--------------------------------------------------------------------------------------------
vec3 DoMaterialRGB(int m, vec3 pos, vec3 norm, vec3 rd)
{
	vec3 rgb;
	if (m == 1)
	{
		rgb = LightCubes(pos, norm);
	}else
	if (m >= 2)
	{
		rgb = LightCharacters(pos, norm, rd, m);
	}else
	{
		rgb = mix(vec3(.0, .05, .1), vec3(0.4, 0.6, .8), abs(rd.y*1.));
    }
	return rgb;
}

//--------------------------------------------------------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	// For Q*Bert's movement to loop properly they needed a direction table...
	//	1	0
	//	 \ /
	//	  @
	//	 / \
	//	3	2
	
	DirTable[0] = 0; DirTable[1] = 2; DirTable[2] = 0; DirTable[3] = 2;
	DirTable[4] = 0; DirTable[5] = 2; DirTable[6] = 0; DirTable[7] = 1;  DirTable[8] = 1; DirTable[9] = 3;  DirTable[10] = 1;
	DirTable[11] = 1; DirTable[12] = 3; DirTable[13] = 3; DirTable[14] = 3;
	DirTable[15] = 3; DirTable[16] = 2; DirTable[17] = 0; DirTable[18] = 0;
	
    time = iGlobalTime*1.8878;;
        
	vec2 pixel = (fragCoord.xy / iResolution.xy)*2.0-1.0;
    float asp = iResolution.x / iResolution.y;
    vec3 rd = normalize(vec3(asp*pixel.x, pixel.y-1.23, -3.5));
    vec3 ro = vec3(-14.0, 6.3, 14.0);
	// Rotate it to look at the plane
	rd.xz = rotate2D(rd.xz, -(PI/4.0));

	float sec, fra;
	vec3 rgb;
    vec3 norm, pos;
	int material; 
	vec3 pos1, pos2;
	radius[0] = .4;
	radius[1] = .33;
	radius[2] = .25;
	for (int i = 0; i < 3; i++)
	{
		sec = time * float(i+3) * .2;
		fra = fract(sec);
		sec = floor(sec);
	
		GetLocation(i, sec, pos1, pos2);
		balls[i] = mix(pos1, pos2, fra);
		balls[i].y += sin(fra*PI);
		if (balls[i].y > -2.5)
		{
			float t = clamp(-fra*.75+1.3, 1.0, 1.3);
			squish[i] = t;
		}else
		{
			squish[i] = 1.0;
		}
	}
	
		
	fra = fract(time*.8);
	sec = floor(time*.8);
	QbertRotation = GetQbertLocation(sec, pos1, pos2);
	
	float t = clamp(fra*.5, 0.0, 0.2)-.3;
	squish[3] = t;
	QbertPos = mix(pos1, pos2, fra); 
	QbertPos.y += sin(fra*PI)+.5;


	TraceEverything(ro, rd, material, norm, pos);
	rgb = DoMaterialRGB(material, pos, norm, rd);
	
	// Do the shadow casting of the balls...
	if (dot(norm, lightDir) > 0.1 && material > 0 && TraceShadow(pos+lightDir*.04, lightDir) != 0)
	{
		rgb *= .4;
	}
	
	
#ifdef REFLECTIONS_ON
	if (material > 0 && material != 6)
	{
		ro = pos;
		rd = ((-2.0*(dot(rd, norm))*norm)+rd);
		TraceEverything(ro+rd*0.04, rd, material, norm, pos);
		rgb = mix(rgb, DoMaterialRGB(material, pos, norm, rd), .13);
	}
#endif	
	
	// Curve the brightness a little...
	rgb = pow(rgb, vec3(.8, .8, .8));

    fragColor=vec4(rgb, 1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
