#version 450
/*
	Quasi Infinite Zoom Voronoi
	---------------------------

	The infinite zoom effect has been keeping me amused for years.

	This one is based on something I wrote some time ago, but was inspired by Fabrice Neyret's 
	"Infinite Fall" shader. I've aired on the side of caution and called it "quasi infinite," 
	just in case it doesn't adhere to his strict infinite zoom standards. :)

	Seriously though, I put together a couple of overly optimized versions a couple of days ago,
	just for fun, and Fabrice's comments were pretty helpful. I also liked the way he did the 
	layer rotation in his "Infinite Fall" version, so I'm using that. The rest is stock standard 
	infinite zoom stuff that has been around for years.

	Most people like to use noise for this effect, so I figured I'd do something different
	and use Voronoi. I've also bump mapped it, added specular highlights, etc. It was 
	tempting to add a heap of other things, but I wanted to keep the example relatively simple.

	By the way, most of the code is basic bump mapping and lighting. The infinite zoom code 
	takes up just a small portion.
	

	Fabrice Neyret's versions:

	infinite fall - short
	https://www.shadertoy.com/view/ltjXWW

    infinite fall - FabriceNeyret2
    https://www.shadertoy.com/view/4sl3RX

	Other examples:

    Fractal Noise - mu6k
    https://www.shadertoy.com/view/Msf3Wr

    Infinite Sierpinski - gleurop
    https://www.shadertoy.com/view/MdfGR8

    Infinite Zoom - fizzer
    https://www.shadertoy.com/view/MlXGW7

	Private link to a textured version of this.
	Bumped Infinite Zoom Texture - Shane
	https://www.shadertoy.com/view/Xl2XWw
*/


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

	Fractal Flythrough
	------------------

	Moving a camera through a fractal object. It's a work in progress.

	I was looking at one of Dr2's shaders that involved moving a camera through a set of way points (set
	out on the XZ plane), and thought it'd be cool to do a similar 3D version. The idea was to create a
	repetitive kind of fractal object, give the open space nodes a set random direction, create some 
	spline points, then run a smooth camera through them. Simple... right? It always seems simple in my
	head, but gets progressively harder when I try it in a shader. :)

	I've run into that classic up-vector, camera flipping problem... At least, I think that's the problem? 
	Anyway, I'm hoping the solution is simple, and that someone reading this will be able to point me in 
	the right direction.

	For now, I've set up a set of 16 random looping points that the camera seems reasonably comfortable 
	with. Just for the record, the general setup works nicely, until the camera loops back on itself in 
	the YZ plane. I'm guessing that increasing the number of way points may eradicate some of the
    intermittent camera spinning, but I figured I'd leave things alone and treat it as a feature. :)

	By the way, I was thankful to have Otavio Good's spline setup in his "Alien Beacon" shader as a 
	reference. On a side note, that particular shader is one of my all time favorites on this site.

	The rendering materials are slightly inspired by the Steampunk genre. Timber, granite, brass, etc. 
	It needs spinning turbines, gears, rivots, and so forth, but that stuff's expensive. Maybe later. 
	Tambako	Jaguar did a really cool shader in the Steampunk aesthetic. The link is below.

	Besides camera path, there's a whole bunch of improvements I'd like to make to this. I've relied on
	occlusion to mask the fact that there are no shadows. I'm hoping to free up some cycles, so I can put
	them back in. I'd also like to add extra detail, but that also slows things down. As for the comments,
	they're very rushed, but I'll tidy those up as well.

	References:

	Alien Beacon - Otavio Good
	https://www.shadertoy.com/view/ld2SzK

    Steampunk Turbine - TambakoJaguar
    https://www.shadertoy.com/view/lsd3zf
    
    // The main inspiration for this shader.
	Mandelmaze in Daylight - dr2
    https://www.shadertoy.com/view/MdVGRc

*/

const float FAR = 50.0; // Far plane.

// Used to identify individual scene objects. In this case, there are only three: The metal framework, the gold
// and the timber.
float objID = 0.; // Wood = 1., Metal = 2., Gold = 3..

// Simple hash function.
float hash( float n ){ return fract(cos(n)*45758.5453); }



// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch01.html
vec3 tex3D(sampler2D t, in vec3 p, in vec3 n ){
    
    n = max(abs(n), 0.001);
    n /= dot(n, vec3(1));
	vec3 tx = texture(t, p.yz).xyz;
    vec3 ty = texture(t, p.zx).xyz;
    vec3 tz = texture(t, p.xy).xyz;
    
    // Textures are stored in sRGB (I think), so you have to convert them to linear space 
    // (squaring is a rough approximation) prior to working with them... or something like that. :)
    // Once the final color value is gamma corrected, you should see correct looking colors.
    return (tx*tx*n.x + ty*ty*n.y + tz*tz*n.z);
    
}

// Common formula for rounded squares, for all intended purposes.
float lengthN(in vec2 p, in float n){ p = pow(abs(p), vec2(n)); return pow(p.x + p.y, 1.0/n); }


// The camera path: There are a few spline setups on Shadertoy, but this one is a slight variation of
// Otavio Good's spline setup in his "Alien Beacon" shader: https://www.shadertoy.com/view/ld2SzK
//
// Spline point markers ("cp" for camera point). The camera visits each point in succession, then loops
// back to the first point, when complete, in order to repeat the process. In case it isn't obvious, each 
// point represents an open space juncture in the object that links to the previous and next point.
// Of course, running a camera in a straight line between points wouldn't produce a smooth camera effect, 
// so we apply the Catmull-Rom equation to the line segment.
vec3 cp[16];

void setCamPath(){
    
    // The larger fractal object has nodes in a 4x4x4 grid.
    // The smaller one in a 2x2x2 grid. The following points
    // map a path to various open areas throughout the object.
    const float sl = 2.*.96;
    const float bl = 4.*.96;
    
    cp[0] = vec3(0, 0, 0);
    cp[1] = vec3(0, 0, bl);
    cp[2] = vec3(sl, 0, bl);
    cp[3] = vec3(sl, 0, sl);
    cp[4] = vec3(sl, sl, sl);	
    cp[5] = vec3(-sl, sl, sl);	
    cp[6] = vec3(-sl, 0, sl);
    cp[7] = vec3(-sl, 0, 0);
    
    cp[8] = vec3(0, 0, 0);	
    cp[9] = vec3(0, 0, -bl);
    cp[10] = vec3(0, bl, -bl);	
    cp[11] = vec3(-sl, bl, -bl);
    cp[12] = vec3(-sl, 0, -bl);
    cp[13] = vec3(-sl, 0, 0);
    cp[14] = vec3(-sl, -sl, 0);
    cp[15] = vec3(0, -sl, 0); 
    
    // Tighening the radius a little, so that the camera doesn't hit the walls.
    // I should probably hardcode this into the above... Done.
    //for(int i=0; i<16; i++) cp[i] *= .96;
    
}

// Standard Catmull-Rom equation. The equation takes in the line segment end points (p1 and p2), the
// points on either side (p0 and p3), the current fractional distance (t) along the segment, then
// returns the the smooth (cubic interpolated) position. The end result is a smooth transition 
// between points... Look up a diagram on the internet. That should make it clearer.
vec3 Catmull(vec3 p0, vec3 p1, vec3 p2, vec3 p3, float t){

    return (((-p0 + p1*3. - p2*3. + p3)*t*t*t + (p0*2. - p1*5. + p2*4. - p3)*t*t + (-p0 + p2)*t + p1*2.)*.5);
   
}
 
// Camera path. Determine the segment number (segNum), and how far - timewise - we are along it (segTime).
// Feed the segment, the appropriate adjoining segments, and the segment time into the Catmull-Rom
// equation to produce a camera position. The process is pretty simple, once you get the hang of it.
vec3 camPath(float t){
    
    const int aNum = 16;
    
    t = fract(t/float(aNum))*float(aNum);	// Repeat every 16 time units.
    
    // Segment number. Range: [0, 15], in this case.
    float segNum = floor(t);
    // Segment portion. Analogous to how far we are alone the individual line segment. Range: [0, 1].
    float segTime = t - segNum; 
    
    
    if (segNum == 0.) return Catmull(cp[aNum-1], cp[0], cp[1], cp[2], segTime); 
    
    for(int i=1; i<aNum-2; i++){
        if (segNum == float(i)) return Catmull(cp[i-1], cp[i], cp[i+1], cp[i+2], segTime); 
    }
    
    if (segNum == float(aNum-2)) return Catmull(cp[aNum-3], cp[aNum-2], cp[aNum-1], cp[0], segTime); 
    if (segNum == float(aNum-1)) return Catmull(cp[aNum-2], cp[aNum-1], cp[0], cp[1], segTime);

    return vec3(0);
    
}

// Smooth minimum function. There are countless articles, but IQ explains it best here:
// http://iquilezles.org/www/articles/smin/smin.htm
float sminP( float a, float b, float s ){

    float h = clamp( 0.5+0.5*(b-a)/s, 0.0, 1.0 );
    return mix( b, a, h ) - s*h*(1.0-h);
}

// Creating the scene geometry.
//
// There are two intertwined fractal objects. One is a gold and timber lattice, spread out in a 4x4x4
// grid. The second is some metallic tubing spread out over a 2x2x2 grid. Each are created by combining
// repeat objects with various operations. All of it is pretty standard.
//
// The code is a little fused together, in order to save some cycles, but if you're interested in the 
// process, I have a "Menger Tunnel" example that's a little easier to decipher.
float map(in vec3 q){

    
///////////

    // The grey section. I have another Menger example, if you'd like to look into that more closely.
    // Layer one.
 	vec3 p = abs(fract(q/4.)*4. - 2.);
 	float tube = min(max(p.x, p.y), min(max(p.y, p.z), max(p.x, p.z))) - 4./3. - .015;// + .05;
    

    // Layer two.
    p = abs(fract(q/2.)*2. - 1.);
 	//d = max(d, min(max(p.x, p.y), min(max(p.y, p.z), max(p.x, p.z))) - s/3.);// + .025
 	tube = max(tube, sminP(max(p.x, p.y), sminP(max(p.y, p.z), max(p.x, p.z), .05), .05) - 2./3.);// + .025
   
///////
    // The gold and timber paneling.
    //
    // A bit of paneling, using a combination of repeat objects. We're doing it here in layer two, just
    // to save an extra "fract" call. Very messy, but saves a few cycles... maybe.
    
    //float panel = sminP(length(p.xy),sminP(length(p.yz),length(p.xz), 0.25), 0.125)-0.45; // EQN 1
    //float panel = sqrt(min(dot(p.xy, p.xy),min(dot(p.yz, p.yz),dot(p.xz, p.xz))))-0.5; // EQN 2
    //float panel = min(max(p.x, p.y),min(max(p.y, p.z),max(p.x, p.z)))-0.5; // EQN 3
    float panel = sminP(max(p.x, p.y),sminP(max(p.y, p.z),max(p.x, p.z), .125), .125)-0.5; // EQN 3
    

    // Gold strip. Probably not the best way to do this, but it gets the job done.
    // Identifying the gold strip region, then edging it out a little... for whatever reason. :)
    float strip = step(p.x, .75)*step(p.y, .75)*step(p.z, .75);
    panel -= (strip)*.025;     
    
    // Timber bulge. Just another weird variation.
    //float bulge = (max(max(p.x, p.y), p.z) - .55);//length(p)-1.;//
    //panel -= bulge*(1.-step(p.x, .75)*step(p.y, .75)*step(p.z, .75))*bulge*.25;    
    
    // Repeat field entity two, which is just an abstract object repeated every half unit. 
    p = abs(fract(q*2.)*.5 - .25);
    float pan2 = min(p.x, min(p.y,p.z))-.05;    
    
    // Combining the two entities above.
    panel = max(abs(panel), abs(pan2)) - .0425;    
/////////
    
    // Layer three. 3D space is divided by three.
    p = abs(fract(q*1.5)/1.5 - 1./3.);
 	tube = max(tube, min(max(p.x, p.y), min(max(p.y, p.z), max(p.x, p.z))) - 2./9. + .025); // + .025 


    // Layer three. 3D space is divided by two, instead of three, to give some variance.
    p = abs(fract(q*3.)/3. - 1./6.);
 	tube = max(tube, min(max(p.x, p.y), min(max(p.y, p.z), max(p.x, p.z))) - 1./9. - .035); //- .025 
    

    
    
    // Object ID: Equivalent to: if(tube<panel)objID=2; else objID = 1.; //etc.
    //
    // By the way, if you need to identify multiple objects, you're better off doing it in a seperate pass, 
    // after the raymarching function. Having multiple "if" statements in a distance field equation can 
    // slow things down considerably.
        
    //objID = 2. - step(tube, panel) + step(panel, tube)*(strip);
    objID = 1.+ step(tube, panel) + step(panel, tube)*(strip)*2.;
    //objID = 1. + step(panel, tube)*(strip) + step(tube, panel)*2.;
    

    return min(panel, tube);
    
    
}

float trace(in vec3 ro, in vec3 rd){

    float t = 0.0, h;
    for(int i = 0; i < 92; i++){
    
        h = map(ro+rd*t);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(abs(h)<0.001*(t*.25 + 1.) || t>FAR) break; // Alternative: 0.001*max(t*.25, 1.)
        t += h*.8;
        
    }

    return t;
}


// The reflections are pretty subtle, so not much effort is being put into them. Only eight iterations.
float refTrace(vec3 ro, vec3 rd){

    float t = 0.0;
    for(int i=0; i<16; i++){
        float d = map(ro + rd*t);
        if (d < 0.0025*(t*.25 + 1.) || t>FAR) break;
        t += d;
    } 
    return t;
}



/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ.
vec3 calcNormal(in vec3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    vec2 e = vec2(0.0025, -0.0025); 
    return normalize(e.xyy * map(p + e.xyy) + e.yyx * map(p + e.yyx) + e.yxy * map(p + e.yxy) + e.xxx * map(p + e.xxx));
}
*/

// Standard normal function. It's not as fast as the tetrahedral calculation, but more symmetrical. Due to 
// the intricacies of this particular scene, it's kind of needed to reduce jagged effects.
vec3 calcNormal(in vec3 p) {
	const vec2 e = vec2(0.005, 0);
	return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
}

// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
float calcAO(in vec3 pos, in vec3 nor)
{
	float sca = 2.0, occ = 0.0;
    for( int i=0; i<5; i++ ){
    
        float hr = 0.01 + float(i)*0.5/4.0;        
        float dd = map(nor * hr + pos);
        occ += (hr - dd)*sca;
        sca *= 0.7;
    }
    return clamp( 1.0 - occ, 0.0, 1.0 );    
}


// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 texBump( sampler2D tx, in vec3 p, in vec3 n, float bf){
   
    const vec2 e = vec2(0.001, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    
    vec3 g = vec3(0.299, 0.587, 0.114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);
                      
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
	
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ){
   
    
	// Screen coordinates.
	vec2 u = (fragCoord - iResolution.xy*0.5)/iResolution.y;
    
    float speed = iGlobalTime*0.35 + 8.;
    
    // Initiate the camera path spline points. Kind of wasteful not making this global, but I wanted
    // it self contained... for better or worse. I'm not really sure what the GPU would prefer.
    setCamPath();

    
	// Camera Setup.
    vec3 ro = camPath(speed); // Camera position, doubling as the ray origin.
    vec3 lk = camPath(speed + .5);  // "Look At" position.
    vec3 lp = camPath(speed + .5) + vec3(0, .25, 0); // Light position, somewhere near the moving camera.
   
    
    // Using the above to produce the unit ray-direction vector.
    float FOV = 1.57; // FOV - Field of view.
    vec3 fwd = normalize(lk-ro);
    vec3 rgt = normalize(vec3(fwd.z, 0, -fwd.x));
    vec3 up = (cross(fwd, rgt));
    
        // Unit direction ray.
    vec3 rd = normalize(fwd + FOV*(u.x*rgt + u.y*up));
    
    
    // Raymarch the scene.
    float t = trace(ro, rd);
    
    // Initialize the scene color.
    vec3 col = vec3(0);
    
    // Scene hit, so color the pixel. Technically, the object should always be hit, so it's tempting to
    // remove this entire branch... but I'll leave it, for now.
    if(t<FAR){
        
        // This looks a little messy and haphazard, but it's really just some basic lighting, and application
        // of the following material properties: Wood = 1., Metal = 2., Gold = 3..
    
        float ts = 1.;  // Texture scale.
        
        // Global object ID. It needs to be saved just after the raymarching equation, since other "map" calls,
        // like normal calculations will give incorrect results. Found that out the hard way. :)
        float saveObjID = objID; 
        
        
        vec3 pos = ro + rd*t; // Scene postion.
        vec3 nor = calcNormal(pos); // Normal.
        vec3 sNor = nor;
        
        
        // Apply some subtle texture bump mapping to the panels and the metal tubing.
        nor = texBump(iChannel0, pos*ts, nor, 0.002); // + step(saveObjID, 1.5)*0.002
    
        // Reflected ray. Note that the normal is only half bumped. It's fake, but it helps
        // taking some of the warping effect off of the reflections.
        vec3 ref = reflect(rd, normalize(sNor*.5 + nor*.5)); 
         
        
		col = tex3D(iChannel0, pos*ts, nor); // Texture pixel at the scene postion.
        
        
        vec3  li = lp - pos; // Point light.
        float lDist = max(length(li), .001); // Surface to light distance.
        float atten = 1./(1.0 + lDist*0.125 + lDist*lDist*.05); // Light attenuation.
        li /= lDist; // Normalizing the point light vector.
        
        float occ = calcAO( pos, nor ); // Occlusion.
		
        float dif = clamp(dot(nor, li), 0.0, 1.0); // Diffuse.
        dif = pow(dif, 4.)*2.;
        float spe = pow(max(dot(reflect(-li, nor), -rd), 0.), 8.); // Object specular.
        float spe2 = spe*spe; // Global specular.
        
        float refl = .35; // Reflection coefficient. Different for different materials.

            

        // Reflection color. Mostly fake.
        // Cheap reflection: Not entirely accurate, but the reflections are pretty subtle, so not much 
        // effort is being put in.
        float rt = refTrace(pos + ref*0.1, ref); // Raymarch from "sp" in the reflected direction.
        float rSaveObjID = objID; // IDs change with reflection. Learned that the hard way. :)
        vec3 rsp = pos + ref*rt; // Reflected surface hit point.
        vec3 rsn = calcNormal(rsp); // Normal at the reflected surface. Too costly to bump reflections.
        vec3 rCol = tex3D(iChannel0, rsp*ts, rsn); // Texel at "rsp."
        vec3 rLi = lp-rsp;
        float rlDist = max(length(rLi), 0.001);
        rLi /= rlDist;
        float rDiff = max(dot(rsn, rLi), 0.); // Diffuse light at "rsp."
        rDiff = pow(rDiff, 4.)*2.;
        float rAtten = 1./(1. + rlDist*0.125 + rlDist*rlDist*.05);
        
        if(rSaveObjID>1.5 && rSaveObjID<2.5){
            rCol = vec3(1)*dot(rCol, vec3(.299, .587, .114))*.7 + rCol*.15;//*.7+.2
            //rDiff *= 1.35;
        }
        if(rSaveObjID>2.5){
             //float rc = dot(rCol, vec3(.299, .587, .114));
             vec3 rFire = pow(vec3(1.5, 1, 1)*rCol, vec3(8, 2, 1.5));//*.5+rc*.5;
             rCol = min(mix(vec3(1.5, .9, .375), vec3(.75, .375, .3), rFire), 2.)*.5 + rCol;         
        }
        
        rCol *= (rDiff + .35)*rAtten; // Reflected color. Not accurate, but close enough.         
        
        
        
        // Grey metal inner tubing.
        if(saveObjID>1.5 && saveObjID<2.5){ 
			
            // Grey out the limestone wall color.
            col = vec3(1)*dot(col, vec3(.299, .587, .114))*.7 + col*.15;
            
            refl = .5;
            //dif *= 1.35;
            //spe2 *= 1.35;
            
        }         
        
        // Gold trimming properties. More effort should probably be put in here.
        // I could just write "saveObjID == 3.," but I get a little paranoid where floats are concerned. :)
        if(saveObjID>2.5){

            // For the screen image, we're interested in the offset height and depth positions. Ie: pOffs.zy.
            
            // Pixelized dot pattern shade.
            //float c = dot(col, vec3(.299, .587, .114));
            
            vec3 fire = pow(vec3(1.5, 1, 1)*col, vec3(8, 2, 1.5));//*.5+c*.5;
            col = min(mix(vec3(1, .9, .375), vec3(.75, .375, .3), fire), 2.)*.5 + col;//
            
            refl = .65;
            //dif *= 1.5;
            //spe2 *= 1.5;
            
        }
        
     
        // Combining everything together to produce the scene color.
        col = col*(dif + .35  + vec3(.35, .45, .5)*spe) + vec3(.7, .9, 1)*spe2 + rCol*refl;
        col *= occ*atten; // Applying occlusion.
       
        
    }

    
    // Applying some very slight fog in the distance. This is technically an inside scene...
    // Or is it underground... Who cares, it's just a shader. :)
    col = mix(min(col, 1.), vec3(0), 1.-exp(-t*t/FAR/FAR*20.));//smoothstep(0., FAR-20., t)
    //col = mix(min(col, 1.), vec3(0), smoothstep(0., FAR-35., t));//smoothstep(0., FAR-20., t)
    
    
    
    // Done.
    fragColor = vec4(sqrt(max(col, 0.)), 1.0);
    
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
  FragCoord.y = -FragCoord.y;
  mainImage(FragColor,FragCoord);
}
