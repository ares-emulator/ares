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
	
    Transparent Lattice
	-------------------

	Just a transparent lattice. Not much different to my other transparent examples, 
	except this one is point lit... In case it needs to be said, a lot of it is faked, 
	so is more of a novelty than anything else. 

	I wrote it some time ago, then forgot about it. I thought I'd put it up just in 
	case it's of any use to anyone. It runs reasonably fast, considering that the 
	lighting is calculated multiple times a pass, but could benefit from a little more 
	tweaking.

	Related shaders:

	Cloudy Spikeball - Duke
    https://www.shadertoy.com/view/MljXDw
    // Port from a demo by Las - Worth watching.
    // http://www.pouet.net/topic.php?which=7920&page=29&x=14&y=9

	Virtually the same thing, but with rounded cubes and less interesting lighting.
	Transparent Cube Field - Shane
	https://www.shadertoy.com/view/ll2SRy
	
*/



// Cheap vec3 to vec3 hash. Works well enough, but there are other ways.
vec3 hash33(vec3 p){ 
    
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}


/*
// Rounded cube field, for comparison. It runs at full speed, believe it or not.
float map(vec3 p){
   
	// Creating the repeat cubes, with slightly convex faces. Standard,
    // flat faced cubes don't capture the light quite as well.
    
    // 3D space repetition.
    p = fract(p)-.5; // + o 
    
    // A bit of roundness. Used to give the cube faces a touch of convexity.
    float r = dot(p, p) - 0.21;
    
    // Max of abs(x), abs(y) and abs(z) minus a constant gives a cube.
    // Adding a little bit of "r," above, rounds off the surfaces a bit.
    p = abs(p); 
	return max(max(p.x, p.y), p.z)*.95 + r*0.25 - 0.21;
   
    
    // Alternative. Egg shapes... kind of.
    //float perturb = sin(p.x*10.)*sin(p.y*10.)*sin(p.z*10.);
	//p += hash33(floor(p))*.15;
	//return length(fract(p)-.5)-0.3 + perturb*0.05;
	
}
*/

/*
// A fake noise looking field. Pretty interesting.
float map(vec3 p){

   
	p = (cos(p*.315*2.5 + sin(p.zxy*.875*2.5)));	    // + iGlobalTime*.5
     
    float n = length(p);
    
    p = sin(p*6. + cos(p.yzx*6.));
    
    return n - 1. - abs(p.x*p.y*p.z)*.05;

    
}
*/

float map(vec3 p){

    
    
    vec2 c;
 
    // SECTION 1
    //
    // Repeat field entity one, which is just some tubes repeated in all directions every 
    // two units, then combined with a smooth minimum function. Otherwise known as a lattice.
    p = abs(fract(p/3.)*3.-1.5);
    //c.x = sminP(length(p.xy),sminP(length(p.yz),length(p.xz), 0.25), 0.25)-0.75; // EQN 1
    //c.x = sqrt(min(dot(p.xy, p.xy),min(dot(p.yz, p.yz),dot(p.xz, p.xz))))-0.75; // EQN 2
    c.x = min(max(p.x, p.y),min(max(p.y, p.z),max(p.x, p.z)))-0.75; // EQN 3
    //p = abs(p); c.x = max(p.x,max(p.y,p.z)) - .5;
    

    // SECTION 2
    //
    // Repeat field entity two, which is just an abstract object repeated every half unit. 
    p = abs(fract(p*4./3.)*.75 - 0.375);
    c.y = min(p.x,min(p.y,p.z)); // EQN 1
    //c.y = min(max(p.x, p.y),min(max(p.y, p.z),max(p.x, p.z)))-0.125; //-0.175, etc. // EQN 2    
    //c.y = max(p.x,max(p.y,p.z)) - .4;
    
    // SECTION 3
    //
    // Combining the two entities above.
    //return length(c)-.1; // EQN 1
    //return max(c.x, c.y)-.05; // EQN 2
    return max(abs(c.x), abs(c.y))*.75 + length(c)*.25 - .1;
    //return max(abs(c.x), abs(c.y))*.75 + abs(c.x+c.y)*.25 - .1;
    //return max(abs(c.x), abs(c.y)) - .1;
    
}




// Not big on accuracy, but lower on operations. Few distance function calls are important
// during volumetric passes.
vec3 calcNormal(in vec3 p, float d) {
	const vec2 e = vec2(0.01, 0);
	return normalize(vec3(d - map(p - e.xyy), d - map(p - e.yxy),	d - map(p - e.yyx)));
}

/*
// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ. Unfortunately, still
// not fast enough in this particular instance.
vec3 calcNormal(in vec3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    vec2 e = vec2(0.0025, -0.0025); 
    return normalize(e.xyy * map(p + e.xyy) + e.yyx * map(p + e.yyx) + e.yxy * map(p + e.yxy) + e.x * map(p + e.xxx));
}
*/


void mainImage( out vec4 fragColor, vec2 fragCoord ) {

    
    // Screen coordinates.
	vec2 uv = (fragCoord.xy - iResolution.xy*.5 )/iResolution.y;
	
    // Unit direction ray. The last term is one of many ways to fish-lens the camera.
    // For a regular view, set "rd.z" to something like "0.5."
    vec3 rd = normalize(vec3(uv, (1.-dot(uv, uv)*.5)*.5)); // Fish lens, for that 1337, but tryhardish, demo look. :)
    
    // There are a few ways to hide artifacts and inconsistencies. Making things go fast is one of them. :)
    // Ray origin, scene color, and surface postion vector.
    vec3 ro = vec3(0., 0., iGlobalTime*1.5), col=vec3(0), sp, sn, lp, ld, rnd;
    
	
    // Swivel the unit ray to look around the scene.
    // Compact 2D rotation matrix, courtesy of Shadertoy user, "Fabrice Neyret."
    vec2 a = sin(vec2(1.5707963, 0) + iGlobalTime*0.375);
    rd.xz = mat2(a, -a.y, a.x)*rd.xz;    
    rd.xy = mat2(a, -a.y, a.x)*rd.xy; 
    
    
    lp = vec3(0, 1, 4);
    lp.xz = mat2(a, -a.y, a.x)*lp.xz;    
    lp.xy = mat2(a, -a.y, a.x)*lp.xy; 
    lp += ro;
    
    
    // Unit ray jitter is another way to hide artifacts. It can also trick the viewer into believing
    // something hard core, like global illumination, is happening. :)
    //rd *= 0.99 + hash33(rd)*0.02;
    
    // Some more randomization, to be used for color based jittering inside the loop.
    rnd = hash33(rd+311.);
    
    
	// Ray distance, bail out layer number, surface distance and normalized accumulated distance.
	float t=0., layers=0., d, aD;
	
	// Light variables.
	float lDist, s, l;
    
    // Surface distance threshold. Smaller numbers gives a thinner membrane, but lessens detail... 
    // hard to explain. It's easier to check it out for yourself.
    float thD = .0125; // + smoothstep(-0.2, 0.2, sin(iGlobalTime*0.75 - 3.14159*0.4))*0.025;
 
	
    // Only a few iterations seemed to be enough. Obviously, more looks better, but is slower.
	for(float i=0.; i<64.; i++)	{
        
        // Break conditions. Anything that can help you bail early usually increases frame rate.
        if(layers>31. || dot(col, vec3(.299, .587, .114)) > 1. || t>16.) break;
        
        // Current ray postion. Slightly redundant here, but sometimes you may wish to reuse
        // it during the accumulation stage.
        sp = ro+rd*t;
		
        d = map(sp); // Distance to nearest point on the noise surface.
        
        // If we get within a certain distance of the surface, accumulate some surface values.
        // Values further away have less influence on the total.
        //
        // aD - Accumulated distance. You could smoothly interpolate it, if you wanted.
        //
        // 1/.(1. + t*t*0.1) - Basic distance attenuation. Feel free to substitute your own.
        
         // Normalized distance from the surface threshold value to our current isosurface value.
        aD = (thD-abs(d)*31./32.)/thD;
        
        // If we're within the surface threshold, accumulate some color.
        // Two "if" statements in a shader loop makes me nervous. I don't suspect there'll be any
        // problems, but if there are, let us know.
        if(aD>0.) { 
        
            
            // Add the accumulated surface distance value, along with some basic falloff using the 
            // camera to light distance, "lDist." There's a bit of color jitter there, too.
            
            sn = calcNormal(sp, d)*sign(d);
            ld = (lp - sp); //vec3(.5773)
            lDist = max(length(ld), .001);
            ld /= lDist;
            s = pow(max(dot(reflect(-ld, sn), -rd), 0.), 8.);
            l = max(dot(ld, sn), 0.);
            

            //float c = dot(sin(sp*128. - cos(sp.yzx*64.)), vec3(.166))+.5;
            col += ((l + .1) + vec3(.5, .7, 1)*s)*aD/(1. + lDist*0.25 + lDist*lDist*0.05)*.2;
            // Failed experiment with color jitter to take out more banding.
            //col += ((l + .05 + fract(rnd + i*27.)*.1) + vec3(.5, .7, 1)*s)*aD/(1. + lDist*0.25 + lDist*lDist*0.05)*.2;
            
            
            // The layer number is worth noting. Accumulating more layers gives a bit more glow.
            // Lower layer numbers allow a quicker bailout. A lot of it is guess work.
            layers++;
            
        }
		
        // Kind of weird the way this works. I think not allowing the ray to hone in properly is
        // the very thing that gives an even spread of values. The figures are based on a bit 
        // of knowledge versus trial and error. If you have a faster computer, feel free to tweak
        // them a bit.
        t += max(abs(d)*.75, thD*.25);
        
			    
	}
	
	t = min(t, 16.);
    
     
    col = mix(col, vec3(0), 1.-exp(-0.025*t*t));////1.-exp(-0.01*t*t) 1.-1./(1. + t*t*.1)
  
    // Mixing the greytone color with a firey orange vignette. There's no meaning
    // behind it. I just thought the artsy greyscale was a little too artsy.
    uv = abs(fragCoord.xy/iResolution.xy - .5); // Wasteful, but the GPU can handle it.
    col = mix(col, vec3(min(col.x*1.5, 1.), pow(col.x, 2.5), pow(col.x, 12.)).yxz,
               min( dot(pow(uv, vec2(4.)), vec2(1))*8., 1.));
               
    //col = vec3(min(col.z*1.5, 1.), pow(col.z, 2.5), pow(col.z, 12.));

    
	// Mixing the vignette colors up a bit more.
    col = mix(col, col.zxy, dot(sin(rd*5.), vec3(.166)) + 0.166);

    
	// Presenting the color to the screen.
	fragColor = vec4( sqrt(clamp(col, 0., 1.)), 1.0 );
    
     
 }

 void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
  FragCoord.y = -FragCoord.y;
  mainImage(FragColor,FragCoord);
}