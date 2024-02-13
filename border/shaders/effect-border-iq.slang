#version 450

// All effects created by inigo quilez - iq/2013-2017
// Hexagons - distance : https://www.shadertoy.com/view/Xd2GR3
// Voronoi - smooth    : https://www.shadertoy.com/view/ldB3zc
// Color grid          : https://www.shadertoy.com/view/4dBSRK
// Voronoi - metrics   : https://www.shadertoy.com/view/MdSGRc
// Iterations - shiny  : https://www.shadertoy.com/view/MslXz8
// Fractal Tiling      : https://www.shadertoy.com/view/Ml2GWy
// Bubbles             : https://www.shadertoy.com/view/4dl3zn
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// modified for slang border shader by hunterk

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	uint FrameCount;
	float aspect_x;
	float aspect_y;
	float integer_scale;
	float overscale;
	float border_zoom;
	float border_speed;
	float effect;
	float scanline_toggle;
	float interp_toggle;
	float THICKNESS;
	float DARKNESS;
	float OS_MASK_TOP;
	float OS_MASK_BOTTOM;
	float OS_MASK_LEFT;
	float OS_MASK_RIGHT;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter effect "Effect Choice" 1.0 1.0 7.0 1.0
#pragma parameter aspect_x "Aspect Ratio Numerator" 64.0 1.0 256. 1.0
#pragma parameter aspect_y "Aspect Ratio Denominator" 49.0 1.0 256. 1.0
#pragma parameter integer_scale "Force Integer Scaling" 1.0 0.0 1.0 1.0
#pragma parameter overscale "Integer Overscale" 0.0 0.0 1.0 1.0
#pragma parameter border_zoom "Border Zoom" 8.0 0.5 16.0 0.5
#pragma parameter border_speed "Border Animation Speed" 0.5 0.5 10 0.5
#pragma parameter interp_toggle "Sharpen Linear Scaling" 0.0 0.0 1.0 1.0
#pragma parameter scanline_toggle "Scanline Toggle" 0.0 0.0 1.0 1.0
#pragma parameter THICKNESS "Scanline Thickness" 2.0 1.0 12.0 1.0
#pragma parameter DARKNESS "Scanline Darkness" 0.35 0.0 1.0 0.05
#pragma parameter OS_MASK_TOP "Overscan Mask Top" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_BOTTOM "Overscan Mask Bottom" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_LEFT "Overscan Mask Left" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_RIGHT "Overscan Mask Right" 0.0 0.0 1.0 0.005

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 border_coord;
layout(location = 1) out vec2 screen_coord;

void main()
{
	gl_Position = global.MVP * Position;
	vec2 out_res = params.OutputSize.xy;
	vec2 corrected_size = params.SourceSize.xy * vec2(params.aspect_x / params.aspect_y, 1.0)
		 * vec2(params.SourceSize.y / params.SourceSize.x, 1.0);
	float full_scale = (params.integer_scale > 0.5) ? floor(params.OutputSize.y /
		params.SourceSize.y) + params.overscale : params.OutputSize.y / params.SourceSize.y;
	vec2 scale = (params.OutputSize.xy / corrected_size) / full_scale;
	vec2 middle = vec2(0.49999, 0.49999);
	vec2 diff = TexCoord.xy - middle;
	screen_coord = middle + diff * scale;
	border_coord = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 border_coord;
layout(location = 1) in vec2 screen_coord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D Original;
layout(set = 0, binding = 4) uniform sampler2D iChannel0;

vec4 fractal_tiling(vec2 iResolution, vec2 fragCoord)
{
    vec2 pos = 256.0*fragCoord.xy/iResolution.x + params.FrameCount / (15.0 / params.border_speed);

    vec3 col = vec3(0.0);
    for( int i=0; i<6; i++ ) 
    {
        vec2 a = floor(pos);
        vec2 b = fract(pos);
        
        vec4 w = fract((sin(a.x*7.0+31.0*a.y + 0.01* params.FrameCount / (15.0 / params.border_speed))+vec4(0.035,0.01,0.0,0.7))*13.545317); // randoms
                
        col += w.xyz *                                   // color
               smoothstep(0.45,0.55,w.w) *               // intensity
               sqrt( 16.0*b.x*b.y*(1.0-b.x)*(1.0-b.y) ); // pattern
        
        pos /= 2.0; // lacunarity
        col /= 2.0; // attenuate high frequencies
    }
    
    col = pow( 2.5*col, vec3(1.0,1.0,0.7) );    // contrast and color shape
    
    return vec4( col, 1.0 );
}

vec4 bubbles(vec2 iResolution, vec2 fragCoord)
{
	vec2 uv = -1.0 + 2.0*fragCoord.xy / iResolution.xy;
	uv.x *=  iResolution.x / iResolution.y;

    // background	 
	vec3 color = vec3(0.8 + 0.2*uv.y);

    // bubbles	
	for( int i=0; i<20; i++ )
	{
        // bubble seeds
		float pha =      sin(float(i)*546.13+1.0)*0.5 + 0.5;
		float siz = pow( sin(float(i)*651.74+5.0)*0.5 + 0.5, 4.0 );
		float pox =      sin(float(i)*321.55+4.1) * iResolution.x / iResolution.y;

        // buble size, position and color
		float rad = 0.1 + 0.5*siz;
		vec2  pos = vec2( pox, -1.0 * (-1.0-rad + (2.0+2.0*rad)*mod(pha+0.1*(params.FrameCount / (15.0 / params.border_speed))*(0.2+0.8*siz),1.0)));
		float dis = length( uv - pos );
		vec3  col = mix( vec3(0.94,0.3,0.0), vec3(0.1,0.4,0.8), 0.5+0.5*sin(float(i)*1.2+1.9));
		//    col+= 8.0*smoothstep( rad*0.95, rad, dis );
		
        // render
		float f = length(uv-pos)/rad;
		f = sqrt(clamp(1.0-f*f,0.0,1.0));
		color -= col.zyx *(1.0-smoothstep( rad*0.95, rad, dis )) * f;
	}

    // vigneting	
	color *= sqrt(1.5-0.5*length(uv));

	return vec4(color,1.0);
}

vec4 hexagon( vec2 p ) 
{
	vec2 q = vec2( p.x*2.0*0.5773503, p.y + p.x*0.5773503 );
	
	vec2 pi = floor(q);
	vec2 pf = fract(q);

	float v = mod(pi.x + pi.y, 3.0);

	float ca = step(1.0,v);
	float cb = step(2.0,v);
	vec2  ma = step(pf.xy,pf.yx);
	
    // distance to borders
	float e = dot( ma, 1.0-pf.yx + ca*(pf.x+pf.y-1.0) + cb*(pf.yx-2.0*pf.xy) );

	// distance to center	
	p = vec2( q.x + floor(0.5+p.y/1.5), 4.0*p.y/3.0 )*0.5 + 0.5;
	float f = length( (fract(p) - 0.5)*vec2(1.0,0.85) );		
	
	return vec4( pi + ca - cb*ma, e, f );
}

float hash( vec2  p )
{
	float n = dot(p,vec2(127.1,311.7) );
	return fract(sin(n)*43758.5453);
}

float hash1( float n )
{
	return fract(sin(n)*43758.5453);
}

vec2  hash2( vec2  p )
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return fract(sin(p)*43758.5453);
}

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = texture(iChannel0, (uv+0.5) / 32.0, -1.0).yx;
	return mix( rg.x, rg.y, f.z );
}

vec4 hexagons_bg(vec2 coord, vec2 output_size, float frame_count)
{
    vec2 uv = coord;
	vec2 pos = (-output_size.xy + 8.0 * (coord * output_size.xy))/ output_size.y;
	float timer = frame_count * 0.05 * params.border_speed;
	
    // distort
	pos *= 1.0 + 0.9*length(pos);
	
    // gray
	vec4 h = hexagon(8.0*pos + 0.5*timer);
	float n = noise( vec3(0.3*h.xy+timer*0.1,timer) );
	vec3 col = 0.15 + 0.15*hash(h.xy+1.2)*vec3(1.0);
	col *= smoothstep( 0.10, 0.11, h.z );
	col *= smoothstep( 0.10, 0.11, h.w );
	col *= 1.0 + 0.15*sin(40.0*h.z);
	col *= 0.75 + 0.5*h.z*n;

	// red
	h = hexagon(6.0*pos + 0.6*timer);
	n = noise( vec3(0.3*h.xy+timer*0.1, timer) );
	vec3 colb = 0.9 + 0.8*sin( hash(h.xy)*1.5 + 2.0 + vec3(0.0,1.0,1.0) );
	colb *= smoothstep( 0.10, 0.11, h.z );
	colb *= 1.0 + 0.15*sin(40.0*h.z);
	colb *= 0.75 + 0.5*h.z*n;

	h = hexagon(6.0*(pos+0.1*vec2(-1.3,1.0)) + 0.6*timer);
    col *= 1.0-0.8*smoothstep(0.45,0.451,noise( vec3(0.3*h.xy+timer*0.1,timer) ));

	col = mix( col, colb, smoothstep(0.45,0.451,n) );

	
	col *= pow( 16.0*uv.x*(1.0-uv.x)*uv.y*(1.0-uv.y), 0.1 );
	return vec4(col, 1.0);
}

vec4 voronoi_1( in vec2 x, float w, float frame_count )
{
    vec2 n = floor( x );
    vec2 f = fract( x );

	vec4 m = vec4( 8.0, 0.0, 0.0, 0.0 );
    for( int j=-1; j<=2; j++ )
    for( int i=-1; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
        vec2 o = vec2(hash( n + g ));
		
		// animate
        o = 0.5 + 0.5*sin( 0.01 * frame_count + 6.2831*o );

        // distance to cell		
		float d = length(g - f + o);
		
        // do the smooth min for colors and distances		
		vec3 col = 0.5 + 0.5*sin( hash1(dot(n+g,vec2(7.0,113.0)))*2.5 + 3.5 + vec3(2.0,3.0,0.0));
		float h = smoothstep( 0.0, 1.0, 0.5 + 0.5*(m.x-d)/w );
		
	    m.x   = mix( m.x,     d, h ) - h*(1.0-h)*w/(1.0+3.0*w); // distance
		m.yzw = mix( m.yzw, col, h ) - h*(1.0-h)*w/(1.0+3.0*w); // cioloe
    }
	
	return m;
}

vec4 voronoi_smooth(vec2 texture_size, float frame_count, vec2 uv)
{
    vec2 p = uv * vec2(2.0,1.0);
	
	float k = 2.0 + 70.0 * pow( 0.5 + 0.5*sin(0.25*6.2831*0.03 * frame_count), 4.0 );
	k = 0.5 - 0.5*cos(0.25*6.2831*0.01 * frame_count);
    vec4 c = voronoi_1( 6.0*p, k, frame_count );

    vec3 col = c.yzw;
	
	col *= 1.0 - 0.8*c.x*step(p.y,0.33);
	col *= mix(c.x,1.0,step(p.y,0.66));
	
	col *= smoothstep( 0.005, 0.007, abs(p.y-0.33) );
	col *= smoothstep( 0.005, 0.007, abs(p.y-0.66) );
	
	return vec4( col, 1.0 );
}

vec4 voronoi_2( in vec2 x, float mode, float timer )
{
    vec2 n = floor( x );
    vec2 f = fract( x );

	vec3 m = vec3( 8.0 );
	float m2 = 8.0;
    for( int j=-1; j<=2; j++ )
    for( int i=-1; i<=2; i++ )
    {
        vec2 g = vec2( float(i),float(j) );
        vec2 o = hash2( n + g );

		// animate
        o = 0.5 + 0.5*sin( timer + 6.2831*o );

		vec2 r = g - f + o;

        // euclidean		
		vec2 d0 = vec2( sqrt(dot(r,r)), 1.0 );
        // manhattam		
		vec2 d1 = vec2( 0.71*(abs(r.x) + abs(r.y)), 1.0 );
        // triangular		
		vec2 d2 = vec2( max(abs(r.x)*0.866025+r.y*0.5,-r.y), 
				        step(0.0,0.5*abs(r.x)+0.866025*r.y)*(1.0+step(0.0,r.x)) );

		vec2 d = d0; 
		if( mode<3.0 ) d=mix( d2, d0, fract(mode) );
		if( mode<2.0 ) d=mix( d1, d2, fract(mode) );
		if( mode<1.0 ) d=mix( d0, d1, fract(mode) );
		
        if( d.x<m.x )
        {
			m2 = m.x;
            m.x = d.x;
            m.y = hash1( dot(n+g,vec2(7.0,113.0) ) );
			m.z = d.y;
        }
		else if( d.x<m2 )
		{
			m2 = d.x;
		}

    }
    return vec4( m, m2-m.x );
}

vec4 voronoi_metrics(vec2 texture_size, float frame_count, vec2 uv)
{
	float mode = 2.0;//mod(frame_count/5.0,3.0);
//	mode = floor(mode) + smoothstep( 0.8, 1.0, fract(mode) );
	
    vec2 p = -uv.xy/texture_size.xx;
    vec4 c = voronoi_2( 8.0*p, mode, frame_count / 4.0 );

    vec3 col = 0.5 + 0.5*sin( c.y*2.5 + vec3(1.5,1.0,1.0) );
    col *= sqrt( clamp( 1.0 - c.x, 0.0, 1.0 ) );
	col *= clamp( 0.5 + (1.0-c.z/2.0)*0.5, 0.0, 1.0 );
	col *= 0.4 + 0.6*sqrt(clamp( 4.0*c.w, 0.0, 1.0 ));
	
	
    return vec4( col, 1.0 );
}

vec4 colorgrid(vec2 texture_size, float frame_count, vec2 uv)
{
    vec2  px = 1000.0 *(-texture_size.xy + 2.0 * params.border_zoom * uv) / texture_size.y / 5.0;
	
    float id = 0.5 + 0.5*cos(frame_count * 0.01 * params.border_speed + sin(dot(floor(px+0.5),vec2(113.1,17.81)))*43758.545);
    
    vec3  co = 0.5 + 0.5*cos(frame_count * 0.01 + 3.5*id + vec3(0.0,1.57,3.14) );
    
    vec2  pa = smoothstep( 0.0, 0.2, id*(0.5 + 0.5*cos(6.2831*px)) );

	return vec4( co*pa.x*pa.y, 1.0 );
}

vec4 shiny_iterations(vec2 texture_size, float frame_count, vec2 uv)
{
	vec2 pc = uv * 5.0;

	vec2 pa = pc + vec2(0.04,0.0);
	vec2 pb = pc + vec2(0.0,0.04);
	
    // shape (3 times for diferentials)
	vec2 zc = pc;
	vec3 fc = vec3( 0.0, 0.0, 0.0 );
	for( int i=0; i<8; i++ ) 
	{
        // transform		
		zc += cos(zc.yx + cos(zc.yx + cos(zc.yx+frame_count * 0.01 * params.border_speed) ) );

        // orbit traps		
		float d = dot( zc-pc, zc-pc ); 
		fc.x += 1.0/(1.0+d);
		fc.y += d;
		fc.z += sin(atan(zc.x-pc.x, zc.y-pc.y));
	}
	fc /= 8.0;
	vec3 sc = fc;
	
	vec2 za = pa;
	vec3 fa = vec3( 0.0, 0.0, 0.0 );
		for( int j=0; j<8; j++ ) 
	{
        // transform		
		za += cos(za.yx + cos(za.yx + cos(za.yx+frame_count * 0.01 * params.border_speed) ) );

        // orbit traps		
		float d = dot( za-pa, za-pa ); 
		fa.x += 1.0/(1.0+d);
		fa.y += d;
		fa.z += sin(atan(zc.x-pc.x, zc.y-pc.y));
	}
	fa /= 8.0;
	vec3 sa = fa;
	
	vec2 zb = pb;
	vec3 fb = vec3( 0.0, 0.0, 0.0 );
		for( int k=0; k<8; k++ ) 
	{
        // transform		
		zb += cos(zb.yx + cos(zb.yx + cos(zb.yx+frame_count * 0.01 * params.border_speed) ) );

        // orbit traps		
		float d = dot( zb-pb, zb-pb ); 
		fb.x += 1.0/(1.0+d);
		fb.y += d;
		fb.z += sin(atan(zc.x-pc.x, zc.y-pc.y));
	}
	fb /= 8.0;
	vec3 sb = fb;

    // color	
	vec3 col = mix( vec3(0.08,0.02,0.15), vec3(0.6,1.1,1.6), sc.x );
	col = mix( col, col.zxy, smoothstep(-0.5,0.5,cos(0.01*frame_count * 0.5)) );
	col *= 0.15*sc.y;
	col += 0.4*abs(sc.z) - 0.1;

    // light	
	vec3 nor = normalize( vec3( sa.x-sc.x, 0.01, sb.x-sc.x ) );
	float dif = clamp(0.5 + 0.5*dot( nor,vec3(0.5773, 0.5773, 0.5773) ),0.0,1.0);
	col *= 1.0 + 0.7*dif*col;
	col += 0.3 * pow(nor.y,128.0);

    // vignetting	
	col *= 1.0 - 0.1*length(pc);
	return vec4(col, 1.0);
}

vec4 scanlines(vec4 frame, vec2 coord, vec2 texture_size, vec2
	video_size, vec2 output_size)
{
	float lines = fract(coord.y * texture_size.y);
	float scale_factor = floor((output_size.y / video_size.y) + 0.4999);
    float lightness = 1.0 - params.DARKNESS;
	return (params.scanline_toggle > 0.5 && (lines < (1.0 / scale_factor * params.THICKNESS)))
		? frame * vec4(lightness, lightness, lightness, lightness) : frame;
}

vec2 interp_coord(vec2 coord, vec2 texture_size)
{
	vec2 p = coord.xy;
	p = p * texture_size.xy + vec2(0.5, 0.5);
	vec2 i = floor(p);
	vec2 f = p - i;
	// Smoothstep - amazingly, smoothstep() is slower than calculating directly the expression!
	f = f * f * f * f * (f * (f * (-20.0 * f + vec2(70.0, 70.0)) - vec2(84.0, 84.0)) + vec2(35.0, 35.0));
	p = i + f;
	p = (p - vec2(0.5, 0.5)) * 1.0 / texture_size;
	return p;
}

vec4 border(vec2 screen_coord, vec2 border_coord, vec2 texture_size, vec2 video_size,
	vec2 output_size, float frame_count, sampler2D decal)
{
	vec4 background;
	if (params.effect == 1.0) background = hexagons_bg(border_coord * (2.0 / params.border_zoom), output_size.xy, frame_count);
	else if (params.effect == 2.0) background = voronoi_smooth(texture_size, frame_count, border_coord);
	else if (params.effect == 3.0) background = colorgrid(texture_size, frame_count * 6.0, border_coord / vec2(output_size.y / output_size.x, 1.0));
	else if (params.effect == 4.0) background = shiny_iterations(texture_size, frame_count, border_coord);
	else if (params.effect == 5.0) background = bubbles(texture_size.xy, border_coord / vec2(output_size.y / output_size.x, 1.0) * (15.0 * params.border_zoom));
	else if (params.effect == 6.0) background = voronoi_metrics(texture_size, frame_count / 10 * params.border_speed, border_coord * 75.0 * params.border_zoom);
	else background = fractal_tiling(texture_size.xy, border_coord / vec2(output_size.y / output_size.x, 1.0) * (30.0 * params.border_zoom));
	
	vec2 coord = (params.interp_toggle < 0.5) ? screen_coord : interp_coord(screen_coord, texture_size);
	vec4 frame = texture(decal, coord);
	frame = scanlines(frame, coord * 1.0001, texture_size, video_size, output_size);
	vec2 fragcoord = (coord.xy);
	if (fragcoord.x < 1.0 - params.OS_MASK_RIGHT && fragcoord.x > 0.0 + params.OS_MASK_LEFT &&
		fragcoord.y < 1.0 - params.OS_MASK_BOTTOM && fragcoord.y > 0.0 + params.OS_MASK_TOP)
			return frame;
	
	else return background;
}

void main()
{
	FragColor = border(screen_coord, border_coord, params.SourceSize.xy, params.SourceSize.xy, 
		params.OutputSize.xy, float(params.FrameCount), Source);
}