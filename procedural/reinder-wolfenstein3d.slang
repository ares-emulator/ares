#version 450
// Wolfenstein. Created by Reinder Nijhoff 2013
// @reindernijhoff

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

#define NUM_MATERIALS 3
#define NUM_OBJECTS 1
#define SECONDS_IN_ROOM 3.
#define ROOM_SIZE 10.
#define MAXSTEPS 17
#define MATERIAL_DOOR 200
#define MATERIAL_DOORWAY 201

#define COL(r,g,b) vec3(r/255.,g/255.,b/255.)

#define time (iGlobalTime+40.)
vec3 rdcenter;

//----------------------------------------------------------------------
// Math functions

float hash( const float n ) {
    return fract(sin(n*14.1234512)*51231.545341231);
}
float hash( const vec2 x ) {
	float n = dot( x, vec2(14.1432,1131.15532) );
    return fract(sin(n)*51231.545341231);
}
float crossp( const vec2 a, const vec2 b ) { return a.x*b.y - a.y*b.x; }
vec3 rotate(vec3 r, float v){ return vec3(r.x*cos(v)+r.z*sin(v),r.y,r.z*cos(v)-r.x*sin(v));}
bool intersectSegment(const vec3 ro, const vec3 rd, const vec2 a, const vec2 b, out float dist, out float u) {
	vec2 p = ro.xz;	vec2 r = rd.xz;
	vec2 q = a-p;	vec2 s = b-a;
	float rCrossS = crossp(r, s);
	
	if( rCrossS == 0.){
		return false;
    } else {
		dist = crossp(q, s) / rCrossS;
		u = crossp(q, r) / rCrossS;
	
		if(0. <= dist && 0. <= u && u <= 1.){
			return true;
        } else {
			return false;
        }
    }
}

//----------------------------------------------------------------------
// Material helper functions

float onCircle( const vec2 c, const vec2 centre, const float radius ) {
	return clamp( 4.*(radius - distance(c,centre)), 0., 1. );
}
float onCircleLine( const vec2 c, const vec2 centre, const float radius ) {
	return clamp( 1.-1.5*abs(radius - distance(c,centre)), 0., 1. );
}
float onLine( const float c, const float b ) {
	return clamp( 1.-abs(b-c), 0., 1. );
}
float onBand( const float c, const float mi, const float ma ) {
	return clamp( (ma-c+1.), 0., 1. )*clamp( (c-mi+1.), 0., 1. );
}
float onLineSegmentX( const vec2 c, const float b, const float mi, const float ma ) {
	return onLine( c.x, b )*onBand( c.y, mi, ma );
}
float onLineSegmentY( const vec2 c, const float b, const float mi, const float ma ) {
	return onLine( c.y, b )*onBand( c.x, mi, ma );
}
float onRect( const vec2 c, const vec2 lt, const vec2 rb ) {
	return onBand( c.x, lt.x, rb.x )*onBand( c.y, lt.y, rb.y );
}
vec3 addBevel( const vec2 c, const vec2 lt, const vec2 rb, const float size, const float strength, const float lil, const float lit, const vec3 col ) {
	float xl = clamp( (c.x-lt.x)/size, 0., 1. ); 
	float xr = clamp( (rb.x-c.x)/size, 0., 1. );	
	float yt = clamp( (c.y-lt.y)/size, 0., 1. ); 
	float yb = clamp( (rb.y-c.y)/size, 0., 1. );	

	return mix( col, col*clamp(1.0+strength*(lil*(xl-xr)+lit*(yb-yt)), 0., 2.), onRect( c, lt, rb ) );
}
vec3 addKnob( const vec2 c, const vec2 centre, const float radius, const float strength, const vec3 col ) {
	vec2 lv = normalize( centre-c );
	return mix( col, col*(1.0+strength*dot(lv,vec2(-0.7071,0.7071))), onCircle(c, centre, radius ) );
}
float stepeq( float a, float b ) { 
	return step( a, b )*step( b, a );
}
//----------------------------------------------------------------------
// Generate materials!

void getMaterialColor( const int material, in vec2 uv, const float decorationHash, out vec3 col ) {	
	vec3 fgcol;
	
	uv = floor( mod(uv+64., vec2(64.)) );
	vec2 uvs = uv / 64.;
	
	// basecolor
	vec3 basecol = vec3( mix(55./255.,84./255.,uvs.y ) );	
	float br = hash(uv);
	col = basecol;
// grey bricks
	if( material == 0 || material == 1 ) {
		vec2 buv = vec2( mod(uv.x+1. + (floor((uv.y+1.) / 16.) * 16.), 32.) , mod( uv.y+1., 16.) );
		float bbr = mix( 190./255., 91./255., (buv.y)/14. ) + 0.05*br;
		if ( buv.x < 2. || buv.y < 2.) {
			bbr = 72./255.; 
		}
		col = vec3(bbr*0.95);
		col = addBevel( buv, vec2(1.,1.), vec2( 31.5, 15.), 2., 0.35, 1., 1., col);
	// blue wall
		if( material == 1 ) {
			col *= 1.3*COL(11.,50.,209.);
			col = mix( col, COL(2.,15.,86.), onBand(uv.y,14.,49.));
			col = mix( col, COL(9.,44.,185.)*(0.9+0.1*br), onBand(uv.y,16.,47.));
			col = mix( col, COL(3.,25.,122.), onBand(uv.y,21.,42.));
			col = addBevel( uv, vec2(-1.,16.), vec2( 65., 21.), 1., 0.35, 1., 1., col);
			col = addBevel( uv, vec2(-1.,43.), vec2( 65., 48.), 1., 0.35, 1., 1., col);
			
			col = mix( col, COL(2.,11.,74.), onRect(uv, vec2(22.,22.), vec2(42.,42.)));		
			col = mix( col, COL(9.,44.,185.)*(0.95+0.1*br), onRect(uv, vec2(22.,23.), vec2(42.,40.)));
			col = addBevel( uv, vec2(22.,23.), vec2(42.,40.), 1., 0.2, -1., 1., col);
			col = mix( col, mix(COL(2.,11.,74.), COL(3.,25.,122.), (uv.x-26.)/3.), onRect(uv, vec2(26.,23.), vec2(29.,29.)));
			col = mix( col, mix(COL(2.,11.,74.), COL(3.,25.,122.), (uv.y-34.)/2.), onRect(uv, vec2(22.,34.), vec2(29.,36.)));
			col = mix( col, mix(COL(2.,11.,74.), COL(3.,25.,122.), (uv.y-27.)/2.), onRect(uv, vec2(35.,27.), vec2(42.,29.)));
			col = mix( col, mix(COL(2.,11.,74.), COL(3.,25.,122.), (uv.y-34.)/8.), onRect(uv, vec2(35.,34.), vec2(38.,42.)));
		}
	}
// wooden wall
	else if( material == 2 ) {
		float mx = mod( uv.x, 64./5. ); 
		float h1 = hash( floor(uv.x/(64./5.)) );
		float h2 = hash( 1.+1431.16*floor(uv.x/(64./5.)) );
		col = mix( COL(115.,75.,43.),COL( 71.,56.,26.), smoothstep( 0.2, 1., (0.7+h2)*abs(mod( h2-uv.y*(0.05+0.1*h2)+(1.+h1+h2)*sin(mx*(0.1+0.2*h2)), 2. )-1.) ) );

		col = mix( col, mix(COL(40.,31.,13.), COL(142.,91.,56.), (uv.x)/2.), step(uv.x,2.) );
		col = mix( col, mix(COL(40.,31.,13.), COL(142.,91.,56.), (uv.x-10.)/2.), step(10.,uv.x)*step(uv.x,12.) );
		col = mix( col, mix(COL(40.,31.,13.), COL(142.,91.,56.), (uv.x-26.)/2.), step(26.,uv.x)*step(uv.x,28.) );
		col = mix( col, mix(COL(40.,31.,13.), COL(142.,91.,56.), (uv.x-40.)/2.), step(40.,uv.x)*step(uv.x,42.) );
		col = mix( col, mix(COL(40.,31.,13.), COL(142.,91.,56.), (uv.x-54.)/2.), step(54.,uv.x)*step(uv.x,56.) );

		col = mix( col, mix(COL(83.,60.,31.), COL(142.,91.,56.), (uv.x- 8.)), step( 8.,uv.x)*step(uv.x,9.) );
		col = mix( col, mix(COL(83.,60.,31.), COL(142.,91.,56.), (uv.x-24.)), step(24.,uv.x)*step(uv.x,25.) );
		col = mix( col, mix(COL(83.,60.,31.), COL(142.,91.,56.), (uv.x-38.)), step(38.,uv.x)*step(uv.x,39.) );
		col = mix( col, mix(COL(83.,60.,31.), COL(142.,91.,56.), (uv.x-52.)), step(52.,uv.x)*step(uv.x,53.) );
		col = mix( col, mix(COL(83.,60.,31.), COL(142.,91.,56.), (uv.x-62.)), step(62.,uv.x) );
		
		col = mix( col, mix(COL(40.,31.,13.), COL(142.,91.,56.), (uv.y)/2.), step(uv.y,2.) );
		col *= 1.-0.3*stepeq(uv.y,3.);
	}
// door
	else if( material == MATERIAL_DOOR ) {
		fgcol = COL(44., 176., 175.)*(0.95+0.15*sin(-0.25+ 4.*((-0.9-uvs.y)/(1.3-0.8*uvs.x)) ) );
		fgcol = addBevel( uv, vec2(-1.,1.), vec2(62.,66.), 2., 0.4, -1., -1., fgcol);
		fgcol = addBevel( uv, vec2( 6.,6.), vec2(57.,57.), 2.25, 0.5, -1., -1., fgcol);	
		fgcol = mix( addKnob( mod( uv, vec2(8.) ), vec2(3.5), 1.65, 0.5, fgcol ), fgcol, onRect( uv,  vec2( 6.,6.), vec2(57.,57.)) ) ;
		
		//knob
		fgcol *= 1.-0.2*onRect( uv, vec2( 13.5, 28.5 ), vec2( 22.5, 44.5 ) );
		fgcol = mix( fgcol, mix( COL(44.,44.,44.),COL(152.,152.,152.), ((uv.x+(43.-uv.y)-15.)/25. ) ), onRect( uv, vec2( 15., 27. ), vec2( 24., 43. ) ) );
		fgcol = addBevel( uv, vec2( 15., 27. ), vec2( 24., 43. ), 1., 0.45, 1., 1., fgcol);	
		fgcol = mix( fgcol, addKnob( mod( uv, vec2(6.) ), vec2(4.25,5.5), 1.15, 0.75, fgcol ), onRect( uv,  vec2( 15., 27. ), vec2( 24., 43. ) ) ) ;

		fgcol *= 1.-0.5*onRect( uv, vec2( 16.5, 33.5 ), vec2( 20.5, 38.5 ) );
		fgcol = mix( fgcol, mix( COL(88.,84.,11.),COL(251.,242.,53.), ((uv.x+(37.-uv.y)-18.)/7. ) ), onRect( uv, vec2( 18., 33. ), vec2( 21., 37. ) ) );
		fgcol = mix( fgcol, COL(0.,0.,0.), onRect( uv, vec2( 19., 34. ), vec2( 20., 35.7 ) ) );

		fgcol *= 1.-0.2*onRect( uv, vec2( 6.5, 29.5 ), vec2( 10.5, 41.5 ) );
		fgcol = mix( fgcol, mix( COL(88.,84.,11.),COL(251.,242.,53.), ((uv.x+(40.-uv.y)-9.)/13. ) ), onRect( uv, vec2( 9., 29. ), vec2( 11., 40. ) ) );
		fgcol = addBevel( uv, vec2( 9., 29. ), vec2( 11., 40. ), 0.75, 0.5, 1., 1., fgcol);	
		
		col = mix( basecol, fgcol, onRect( uv, vec2(1.,1.), vec2(62.,62.) ) );	
	}
// doorway
	else if( material == MATERIAL_DOORWAY ) {
		fgcol = COL(44., 176., 175.)*(0.95+0.15*sin(-0.25+ 4.*((-0.9-uvs.y)/(1.3-0.8*uvs.x)) ) );
		vec2 uvhx = vec2( 32.-abs(uv.x-32.), uv.y );
		fgcol = addBevel( uvhx, vec2(-1.,1.), vec2(28.,66.), 2., 0.4, -1., -1., fgcol);
		fgcol = addBevel( uvhx, vec2( 6.,6.), vec2(23.,57.), 2.25, 0.5, -1., -1., fgcol);	
		fgcol = mix( addKnob( vec2( mod( uvhx.x, 22. ), mod( uvhx.y, 28. )), vec2(3.5), 1.65, 0.5, fgcol ), fgcol, onRect( uvhx,  vec2( 6.,6.), vec2(24.,57.)) ) ;
		fgcol = mix( fgcol, vec3(0.), onRect( uv, vec2( 29., 1.), vec2( 35., 63.) ) );
		col = mix( basecol, fgcol, onRect( uv, vec2(1.,1.), vec2(62.,62.) ) );	
	}
	
// prison door	
	if( decorationHash > 0.93 && material < (NUM_MATERIALS+1) ) {	
		vec4 prisoncoords = vec4(12.,14.,52.,62.);
	// shadow
		col *= 1.-0.5*onRect( uv,  vec2( 11., 13. ), vec2( 53., 63. ) );
	// hinge
		col = mix( col, COL(72.,72.,72.), stepeq(uv.x, 53.)*step( mod(uv.y+2.,25.), 5.)*step(13.,uv.y) );
		col = mix( col, COL(100.,100.,100.), stepeq(uv.x, 53.)*step( mod(uv.y+1.,25.), 3.)*step(13.,uv.y) );
		
		vec3 pcol = vec3(0.)+COL(100.,100.,100.)*step( mod(uv.x-4., 7.), 0. ); 
		pcol += COL(55.,55.,55.)*step( mod(uv.x-5., 7.), 0. ); 
		pcol = addBevel(uv, vec2(0.,17.), vec2(63.,70.), 3., 0.8, 0., -1., pcol);
		pcol = addBevel(uv, vec2(0.,45.), vec2(22.,70.), 3., 0.8, 0., -1., pcol);
		
		fgcol = COL(72.,72.,72.);
		fgcol = addBevel(uv, prisoncoords.xy, prisoncoords.zw+vec2(1.,1.), 1., 0.5, -1., 1., fgcol );
		fgcol = addBevel(uv, prisoncoords.xy+vec2(3.,3.), prisoncoords.zw-vec2(2.,1.), 1., 0.5, 1., -1., fgcol );
		fgcol = mix( fgcol, pcol, onRect( uv, prisoncoords.xy+vec2(3.,3.), prisoncoords.zw-vec2(3.,2.) ) );
		fgcol = mix( fgcol, COL(72.,72.,72.), onRect( uv, vec2(15.,32.5), vec2(21.,44.) ) );
		
		fgcol = mix( fgcol, mix( COL(0.,0.,0.), COL(43.,43.,43.), (uv.y-37.) ), stepeq(uv.x, 15.)*step(37.,uv.y)*step(uv.y,38.) );
		fgcol = mix( fgcol, mix( COL(0.,0.,0.), COL(43.,43.,43.), (uv.y-37.)/3. ), stepeq(uv.x, 17.)*step(37.,uv.y)*step(uv.y,40.) );
		fgcol = mix( fgcol, COL(43.,43.,43.), stepeq(uv.x, 18.)*step(37.,uv.y)*step(uv.y,41.) );
		fgcol = mix( fgcol, mix( COL(0.,0.,0.), COL(100.,100.,100.), (uv.y-37.)/3. ), stepeq(uv.x, 18.)*step(36.,uv.y)*step(uv.y,40.) );
		fgcol = mix( fgcol, COL(43.,43.,43.), stepeq(uv.x, 19.)*step(37.,uv.y)*step(uv.y,40.) );

		fgcol = mix( fgcol, mix( COL(84.,84.,84.), COL(108.,108.,108.), (uv.x-15.)/2. ), stepeq(uv.y, 32.)*step(15.,uv.x)*step(uv.x,17.) );
		fgcol = mix( fgcol, COL(81.,81.,81.), stepeq(uv.y, 32.)*step(20.,uv.x)*step(uv.x,21.) );

		col = mix( col, fgcol, onRect( uv, prisoncoords.xy, prisoncoords.zw ) );
	}
// flag
	else if( decorationHash > 0.63 && material < (NUM_MATERIALS+1) ) {		
		vec2 uvc = uv-vec2(32.,30.);
	
	// shadow	
		vec4 shadowcoords = vec4( 14., 7., 
								  54., max( 56. + sin( uv.x*0.32-1. ),56.) ); 
		col *= 1.-0.3*onRect( uv,  vec2( 6., 6. ), vec2( 61., 7. ) );
		col *= 1.-0.3*clamp( 0.25*(56.-uv.x), 0., 1.)*onRect( uv, shadowcoords.xy, shadowcoords.zw );

	// rod
		col = mix( col, COL(250.,167.,98.), onLineSegmentX( vec2( abs(uv.x-32.), uv.y ), 26., 4., 6.5 ) );
		col = mix( col, COL(251.,242.,53.), onLineSegmentY( uv, 5., 4., 60. ) );
		col = mix( col, COL(155.,76.,17.), onLineSegmentY( uv, 6., 4., 60. ) );
		col = mix( col, COL(202.,96.,25.), onLineSegmentY( vec2( abs(uv.x-32.), uv.y ), 6., 26., 28. ) );
		col = mix( col, COL(251.,242.,53.), onLineSegmentX( vec2( abs(uv.x-32.), uv.y ), 25., 3., 7. ) );
		col = mix( col, COL(252.,252.,217.), onLineSegmentX( vec2( abs(uv.x-32.), uv.y ), 25., 4.3, 5.5 ) );
		col = mix( col, COL(252.,252.,217.), onLineSegmentX( vec2( abs(uv.x-32.), uv.y ), 26., 5.3, 5.5 ) );
		col = mix( col, COL(0.,0.,0.), onLineSegmentY( vec2( abs(uv.x-32.), uv.y ), 6., 18.3, 19.5 ) );

	// flag	
		vec4 flagcoords = vec4( 13., min( 9.5 - pow(5.5* (uvs.x-0.5), 2.), 9. ), 
						    51., max( 55. + sin( uv.x*0.4+2.7 ),55.) ); 
	
		fgcol = COL(249.,41.,27.);
		
		fgcol = mix( fgcol, COL(255.,255.,255.), onBand( min(abs(uvc.x), abs(uvc.y)), 2., 4. ) );
		fgcol = mix( fgcol, COL(72.,72.,72.), onLine( min(abs(uvc.x), abs(uvc.y)), 3. ) );		
		
		fgcol = mix( fgcol, COL(255.,255.,255.), onCircle( uv, vec2(32.,30.), 12.5 ) );	
		fgcol = mix( fgcol, COL(0.,0.,0.), onCircleLine( uv, vec2(32.,30.), 11. ) );	
		fgcol = mix( fgcol, COL(0.,0.,0.), onCircleLine( uv, vec2(32.,30.), 9. ) );
		
		vec2 uvr = vec2( (uvc.x-uvc.y)*0.7071, (uvc.y+uvc.x)*0.7071)*sign( uvc.x+0.5 );
		fgcol = mix( fgcol, COL(72.,72.,72.), onRect( uvr, vec2(-1.,-1.), vec2(1.,4.) ) );
		fgcol = mix( fgcol, COL(72.,72.,72.), onRect( uvr, vec2(-4.2, 4.2), vec2(1.,6.15) ) );
		fgcol = mix( fgcol, COL(72.,72.,72.), onRect( uvr, vec2(-1.,-1.), vec2(4.,1.) ) );
		fgcol = mix( fgcol, COL(72.,72.,72.), onRect( uvr, vec2( 4.2,-1.), vec2(6.15,4.2) ) );
	
		fgcol *= (0.8+0.2*sin( uv.x*0.4+2.7 ));
		fgcol *= (0.8+0.2*clamp( 0.5*(uv.y-7.), 0., 1.));
	
	// mix flag on background
		col = mix( col, fgcol, onRect( uv, flagcoords.xy, flagcoords.zw ) );
	}
	
// fake 8-bit color palette and dithering	
	col = floor( (col+0.5*mod(uv.x+uv.y,2.)/32.)*32.)/32.;
}
bool getObjectColor( const int object, in vec2 uv, inout vec3 icol ) {
	uv = floor( mod(uv, vec2(64.)) );
	vec2 uvs = uv / 64.;
	vec3 col = vec3(20./255.);
	float d;
	
// only a lamp for now
	
	// lamp top
	d = distance( uv*vec2(1.,2.), vec2(28.1, 5.8)*vec2(1.,2.) );
	col = mix( col, mix( COL(41.,250.,46.), COL(13.,99.,12.), clamp( d/8.-0.2, 0., 1.) ), 
			  onCircle(uv, vec2(31.,13.6), 11.7 )*step( uv.y, 6. )); 
	col = mix( col, COL(9.,75.,6.), onCircleLine( uv, vec2(31.,14.), 11.6 ) *
			  step( length(uv-vec2(31.,13.6)), 11.7 )*step( uv.y, 6. ) );
	col = mix( col, COL(100.,100.,100.), onLine( abs(uv.x-31.), 1. )*step( uv.y, 1. ) );
	col = mix( col, COL(140.,140.,140.), onLine( abs(uv.x-31.), 0.25 )*step( uv.y, 1. )*step( 1., uv.y ) );
	
	// lamp bottom
	d = distance( uv*vec2(1.,2.), vec2(30.5, 6.5)*vec2(1.,2.) );
	col = mix( col, mix( COL(41.,250.,46.), COL(13.,99.,12.), clamp( abs(uv.x-31.)/4.-1.25, 0., 1. )), step( abs(uv.x-31.), 9. )*stepeq( uv.y, 7.) );
	col = mix( col, mix( COL(41.,250.,46.), COL(16.,123.,17.), clamp( abs(uv.x-31.)/4.-1.25, 0., 1. )), step( abs(uv.x-31.), 9. )*stepeq( uv.y, 8.) );
	col = mix( col, mix( COL(133.,250.,130.), COL(22.,150.,23.), clamp( abs(uv.x-31.)/4.-0.75, 0., 1. )), step( abs(uv.x-31.), 7. )*stepeq( uv.y, 9.) );

	col = mix( col, mix( COL(255.,251.,187.), col, clamp( d/4.5-0.6, 0., 1.) ), 
			  onCircle(uv, vec2(31.,1.), 10.2 )*step( uv.y, 8. )*step( 7., uv.y )); 
	col = mix( col, mix( COL(255.,255.,255.), col, clamp( d/4.-0.7, 0., 1.) ), 
			  onCircle(uv, vec2(31.,1.), 7.2 )*step( uv.y, 8. )*step( 7., uv.y )); 
		
	// floor
	d = distance( vec2(mod(uv.x, 32.),uv.y)*vec2(1.5,30./3.), vec2(16., 61.5)*vec2(1.5,30./3.) );
	col = mix( col, mix( COL(168.,168.,168.), COL(124.,124.,124.), clamp(d/15.-0.5, 0., 1.) ), step(d,24.5)); 
	col = mix( col, mix( COL(124.,124.,124.), COL(140.,140.,140.), clamp((uv.y-59.)/1., 0., 1.)), step(59.,uv.y)*step(uv.x, 57.)*step(7.,uv.x)); 
	col = mix( col, mix( COL(168.,168.,168.), COL(124.,124.,124.), clamp(abs(32.-uv.x)/10.-2., 0., 1.)), step(uv.y, 62.)*step(62.,uv.y)*step(uv.x, 61.)*step(3.,uv.x)); 
	col = mix( col, mix( COL(152.,152.,152.), COL(124.,124.,124.), clamp(abs(32.-uv.x)/10.-2.25, 0., 1.)), step(uv.y, 61.)*step(61.,uv.y)*step(uv.x, 59.)*step(5.,uv.x)); 

	col = floor( (col)*32.)/32.;
	if( any(notEqual(col, vec3(floor((20./255.)*32.)/32.))) ) {
		icol = col;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------
// Proocedural MAP functions

bool isWall( const vec2 vos ) {
	return vos.y<0.4*ROOM_SIZE || vos.y>2.75*ROOM_SIZE || any( equal( mod( vos, vec2( ROOM_SIZE ) ), vec2(0.,0.) ) );
}
bool isDoor( const vec2 vos ) {
	return isWall(vos) && ((hash(vos)>0.75 &&  any( equal( mod( vos, vec2( ROOM_SIZE*0.5 ) ), vec2(2.) ) )) 
		    || any( equal( mod( vos, vec2( ROOM_SIZE ) ), vec2(ROOM_SIZE*0.5) ) )); 
}
bool isObject( const vec2 vos ) {
	return hash( vos*10. ) > 0.95;
}
bool map( const vec2 vos ) {
	return isObject( vos ) || isWall( vos );
}

//----------------------------------------------------------------------
// Render MAP functions

bool intersectSprite( const vec3 ro, const vec3 rd, const vec3 vos, const vec3 nor, out vec2 uv ) {
	float dist, u;
	vec2 a = vos.xz + nor.zx*vec2(-0.5,0.5) + vec2(0.5, 0.5);
	vec2 b = vos.xz - nor.zx*vec2(-0.5,0.5) + vec2(0.5, 0.5);
	if( intersectSegment( ro, rd, a, b, dist, u) ) {
		uv.x = u; uv.y = 1.-(ro+dist*rd).y;
		if( sign(nor.x)<0. ) uv.x = 1.-uv.x;
		return uv.y>0.&&uv.y<1.;
	}
	return false;
}
int getMaterialId( const vec2 vos ) {
	return int( mod( 521.21 * hash( floor((vos-vec2(0.5))/ROOM_SIZE )  ), float(NUM_MATERIALS)) );
}
bool getColorForPosition( const vec3 ro, const vec3 rd, const vec3 vos, const vec3 pos, const vec3 nor, inout vec3 col ) {	
	vec2 uv;

	if( isWall( vos.xz ) ) {
		if( isDoor( vos.xz ) ) {
			if( intersectSprite( ro, rd, vos+nor*0.03, nor, uv ) ) {
				// open the door
				uv.x -= clamp( 2.-0.75*distance( ro.xz, vos.xz+vec2(0.5) ), 0., 1.);
				if( uv.x > 0. ) {
					getMaterialColor( MATERIAL_DOOR, uv*64., 0., col );
					return true;
				}	
			}	
			return false;
		}
		// a wall is hit
		if( pos.y <= 1. && pos.y >= 0. ) {
			vec2 mpos = vec2( dot(vec3(-nor.z,0.0,nor.x),pos), -pos.y );
    		float sha = 0.6 + 0.4*abs(nor.z);		
			getMaterialColor( isDoor( vos.xz+nor.xz )?MATERIAL_DOORWAY:getMaterialId(vos.xz), mpos*64., hash( vos.xz ), col );
			col *= sha;
			return true;
		}
		return true;
	}
	if( isObject( vos.xz ) && !isWall( vos.xz+vec2(1.,0.) ) && !isWall( vos.xz+vec2(-1.,0.) )
	    && !isWall( vos.xz+vec2(0.,-1.) ) && !isWall( vos.xz+vec2(0.,1.) ) &&
	    intersectSprite( ro, rd, vos, rdcenter, uv ) ) {
		return getObjectColor( 0, uv*64., col );
	}
	return false;
}

bool castRay( const vec3 ro, const vec3 rd, inout vec3 col ) {
	vec3 pos = floor(ro);
	vec3 ri = 1.0/rd;
	vec3 rs = sign(rd);
	vec3 dis = (pos-ro + 0.5 + rs*0.5) * ri;
	
	float res = 0.0;
	vec3 mm = vec3(0.0);
	bool hit = false;
	
	for( int i=0; i<MAXSTEPS; i++ )	{
		if( hit ) continue;
		
		mm = step(dis.xyz, dis.zyx);
		dis += mm * rs * ri;
        pos += mm * rs;
		
		if( map(pos.xz) ) { 
			vec3 mini = (pos-ro + 0.5 - 0.5*vec3(rs))*ri;
			float t = max ( mini.x, mini.z );			
			hit = getColorForPosition( ro, rd, pos, ro+rd*t, -mm*sign(rd), col );
		}
	}
	return hit;
}

//----------------------------------------------------------------------
// Some really ugly code

#define CCOS(a) cos(clamp(a,0.,1.)*1.57079632679)
#define CSIN(a) sin(clamp(a,0.,1.)*1.57079632679)
vec3 path( const float t ) {
	float tmod = mod( t/SECONDS_IN_ROOM, 8. );
	float tfloor = floor( tmod );
	
	vec3 pos = vec3( 4.*ROOM_SIZE*floor(t/(SECONDS_IN_ROOM*8.))+0.5, 0.5, 0.5*ROOM_SIZE+0.5 );	
	return pos + ROOM_SIZE*vec3(
		clamp(tmod,0.,1.)+clamp(tmod-4.,0.,1.)+0.5*(2.+CSIN(tmod-1.)-CCOS(tmod-3.)+CSIN(tmod-5.)-CCOS(tmod-7.)), 0.,
		clamp(tmod-2.,0.,1.)-clamp(tmod-6.,0.,1.)+0.5*(-CCOS(tmod-1.)+CSIN(tmod-3.)+CCOS(tmod-5.)-CSIN(tmod-7.)) );
}


//----------------------------------------------------------------------
// Main

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 q = fragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0*q;
    p.x *= iResolution.x/ iResolution.y;
	
	vec3 ro = path( time );
	vec3 ta = path( time+0.1 );
	
    rdcenter = rotate( normalize( ta - ro), 0.3*cos(time*0.75) );
    vec3 uu = normalize(cross( vec3(0.,1.,0.), rdcenter ));
    vec3 vv = normalize(cross(rdcenter,uu));
    vec3 rd = normalize( p.x*uu + p.y*vv + 2.5*rdcenter );
	
	vec3 col = rd.y>0.?vec3(56./255.):vec3(112./255.);
	castRay( ro, rd, col );
		
	fragColor = vec4( col, 1.0 );
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
