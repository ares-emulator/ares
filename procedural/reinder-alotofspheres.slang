#version 450
// A lot of spheres. Created by Reinder Nijhoff 2013
// @reindernijhoff
// 
// https://www.shadertoy.com/view/lsX3WH
//

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

#define SHADOW
#define REFLECTION


#define RAYCASTSTEPS 40

#define EXPOSURE 0.9
#define EPSILON 0.0001
#define MAXDISTANCE 400.
#define GRIDSIZE 8.
#define GRIDSIZESMALL 5.
#define MAXHEIGHT 10.
#define SPEED 0.5

#define time iGlobalTime

//
// math functions
//

const mat2 mr = mat2 (0.84147,  0.54030,
					  0.54030, -0.84147 );
float hash( float n ) {
	return fract(sin(n)*43758.5453);
}
vec2 hash2( float n ) {
	return fract(sin(vec2(n,n+1.0))*vec2(2.1459123,3.3490423));
}
vec2 hash2( vec2 n ) {
	return fract(sin(vec2( n.x*n.y, n.x+n.y))*vec2(2.1459123,3.3490423));
}
vec3 hash3( float n ) {
	return fract(sin(vec3(n,n+1.0,n+2.0))*vec3(3.5453123,4.1459123,1.3490423));
}
vec3 hash3( vec2 n ) {
	return fract(sin(vec3(n.x, n.y, n+2.0))*vec3(3.5453123,4.1459123,1.3490423));
}
//
// intersection functions
//

bool intersectPlane(vec3 ro, vec3 rd, float height, out float dist) {	
	if (rd.y==0.0) {
		return false;
	}
	
	float d = -(ro.y - height)/rd.y;
	d = min(100000.0, d);
	if( d > 0. ) {
		dist = d;
		return true;
	}
	return false;
}

bool intersectUnitSphere ( in vec3 ro, in vec3 rd, in vec3 sph, out float dist, out vec3 normal ) {
	vec3  ds = ro - sph;
	float bs = dot( rd, ds );
	float cs = dot(  ds, ds ) - 1.0;
	float ts = bs*bs - cs;
	
	if( ts > 0.0 ) {
		ts = -bs - sqrt( ts );
		if( ts>0. ) {
			normal = normalize( (ro+ts*rd)-sph );
			dist = ts;
			return true;
		}
	}
	
	return false;
}

//
// Scene
//

void getSphereOffset( vec2 grid, inout vec2 center ) {
	center = (hash2( grid+vec2(43.12,1.23) ) - vec2(0.5) )*(GRIDSIZESMALL);
}
void getMovingSpherePosition( vec2 grid, vec2 sphereOffset, inout vec3 center ) {
	// falling?
	float s = 0.1+hash( grid.x*1.23114+5.342+754.324231*grid.y );
	float t = 14.*s + time/s;
	
	float y =  s * MAXHEIGHT * abs( cos( t ) );
	vec2 offset = grid + sphereOffset;
	
	center = vec3( offset.x, y, offset.y ) + 0.5*vec3( GRIDSIZE, 2., GRIDSIZE );
}
void getSpherePosition( vec2 grid, vec2 sphereOffset, inout vec3 center ) {
	vec2 offset = grid + sphereOffset;
	center = vec3( offset.x, 0., offset.y ) + 0.5*vec3( GRIDSIZE, 2., GRIDSIZE );
}
vec3 getSphereColor( vec2 grid ) {
	return normalize( hash3( grid+vec2(43.12*grid.y,12.23*grid.x) ) );
}

vec3 trace(vec3 ro, vec3 rd, out vec3 intersection, out vec3 normal, out float dist, out int material) {
	material = 0; // sky
	dist = MAXDISTANCE;
	float distcheck;
	
	vec3 sphereCenter, col, normalcheck;
	
	if( intersectPlane( ro,  rd, 0., distcheck) && distcheck < MAXDISTANCE ) {
		dist = distcheck;
		material = 1;
		normal = vec3( 0., 1., 0. );
		col = vec3( 1. );
	} else {
		col = vec3( 0. );
	}
	
		
	// trace grid
	vec3 pos = floor(ro/GRIDSIZE)*GRIDSIZE;
	vec3 ri = 1.0/rd;
	vec3 rs = sign(rd) * GRIDSIZE;
	vec3 dis = (pos-ro + 0.5  * GRIDSIZE + rs*0.5) * ri;
	vec3 mm = vec3(0.0);
	vec2 offset;
		
	for( int i=0; i<RAYCASTSTEPS; i++ )	{
		if( material > 1 || distance( ro.xz, pos.xz ) > dist+GRIDSIZE ) break;
		vec2 offset;
		getSphereOffset( pos.xz, offset );
		
		getMovingSpherePosition( pos.xz, -offset, sphereCenter );
		
		if( intersectUnitSphere( ro, rd, sphereCenter, distcheck, normalcheck ) && distcheck < dist ) {
			dist = distcheck;
			normal = normalcheck;
			material = 2;
		}
		
		getSpherePosition( pos.xz, offset, sphereCenter );
		if( intersectUnitSphere( ro, rd, sphereCenter, distcheck, normalcheck ) && distcheck < dist ) {
			dist = distcheck;
			normal = normalcheck;
			col = vec3( 2. );
			material = 3;
		}
		mm = step(dis.xyz, dis.zyx);
		dis += mm * rs * ri;
		pos += mm * rs;		
	}
	
	vec3 color = vec3( 0. );
	if( material > 0 ) {
		intersection = ro + rd*dist;
		vec2 map = floor(intersection.xz/GRIDSIZE)*GRIDSIZE;
		
		if( material == 1 || material == 3 ) {
			// lightning
			vec3 c = vec3( -GRIDSIZE,0., GRIDSIZE );
			for( int x=0; x<3; x++ ) {
				for( int y=0; y<3; y++ ) {
					vec2 mapoffset = map+vec2( c[x], c[y] );		
					vec2 offset;
					getSphereOffset( mapoffset, offset );
					vec3 lcolor = getSphereColor( mapoffset );
					vec3 lpos;
					getMovingSpherePosition( mapoffset, -offset, lpos );
					
					float shadow = 1.;
#ifdef SHADOW
					if( material == 1 ) {
						for( int sx=0; sx<3; sx++ ) {
							for( int sy=0; sy<3; sy++ ) {
								if( shadow < 1. ) continue;
								
								vec2 smapoffset = map+vec2( c[sx], c[sy] );		
								vec2 soffset;
								getSphereOffset( smapoffset, soffset );
								vec3 slpos, sn;
								getSpherePosition( smapoffset, soffset, slpos );
								float sd;
								if( intersectUnitSphere( intersection, normalize( lpos - intersection ), slpos, sd, sn )  ) {
									shadow = 0.;
								}							
							}
						}
					}
#endif
					color += col * lcolor * ( shadow * max( dot( normalize(lpos-intersection), normal ), 0.) *
											 (1. - clamp( distance( lpos, intersection )/GRIDSIZE, 0., 1.) ) );
				}
			}
		} else {
			// emitter
			color = (1.5+dot(normal, vec3( 0.5, 0.5, -0.5) )) *getSphereColor( map );
		}
	}
	return color;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 q = fragCoord.xy/iResolution.xy;
	vec2 p = -1.0+2.0*q;
	p.x *= iResolution.x/iResolution.y;
	
	// camera	
	vec3 ce = vec3( cos( 0.232*time) * 10., 6.+3.*cos(0.3*time), GRIDSIZE*(time/SPEED) );
	vec3 ro = ce;
	vec3 ta = ro + vec3( -sin( 0.232*time) * 10., -2.0+cos(0.23*time), 10.0 );
	
	float roll = -0.15*sin(0.5*time);
	
	// camera tx
	vec3 cw = normalize( ta-ro );
	vec3 cp = vec3( sin(roll), cos(roll),0.0 );
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
	vec3 rd = normalize( p.x*cu + p.y*cv + 1.5*cw );
	
	// raytrace
	int material;
	vec3 normal, intersection;
	float dist;
	
	vec3 col = trace(ro, rd, intersection, normal, dist, material);

#ifdef REFLECTION
	if( material > 0 ) {
		vec3 ro = intersection + EPSILON*normal;
		rd = reflect( rd, normal );
		col += 0.05 * trace(ro, rd, intersection, normal, dist, material);
	}
#endif
	
	col = pow( col, vec3(EXPOSURE, EXPOSURE, EXPOSURE) );	
	col = clamp(col, 0.0, 1.0);
	
	// vigneting
	col *= 0.25+0.75*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.15 );
	
	fragColor = vec4( col,1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
