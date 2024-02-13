#version 450

/*
Shader from Godot Shaders - the free shader library.
godotshaders.com/shader/VHS-and-CRT-monitor-effect

This shader is under CC0 license. Feel free to use, improve and 
change this shader according to your needs and consider sharing 
the modified result to godotshaders.com.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	int FrameDirection;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	float godot_scanlines_opacity, godot_scanlines_width, godot_grille_opacity, godot_pixelate, godot_roll, godot_roll_speed,
		godot_roll_size, godot_roll_variation, godot_distort_intensity, godot_noise_opacity, godot_noise_speed, godot_static_noise_intensity,
		godot_aberration, godot_brightness, godot_discolor, godot_warp_amount, godot_clip_warp, godot_vignette_intensity,
		godot_vignette_opacity, godot_moire;
} global;
// useless for slang, but I'm leaving it in just to avoid changing the original code unnecessarily
bool overlay = false;

#pragma parameter godot_scanlines_opacity "Scanlines Opacity" 0.4 0.0 1.0 0.01
float scanlines_opacity = global.godot_scanlines_opacity;
#pragma parameter godot_scanlines_width "Scanlines Width" 0.25 0.0 0.5 0.05
float scanlines_width = global.godot_scanlines_width;
#pragma parameter godot_grille_opacity "Grille Opacity" 0.3 0.0 1.0 0.01
float grille_opacity = global.godot_grille_opacity;

// hook this one up to the actual image resolution
vec2 resolution = params.SourceSize.xy; //vec2(640.0, 480.0); // Set the number of rows and columns the texture will be divided in. Scanlines and grille will make a square based on these values

// another unnecessary one, but we'll leave it for now
bool pixelate = false; // Fill each square ("pixel") with a sampled color, creating a pixel look and a more accurate representation of how a CRT monitor would work.

#pragma parameter godot_roll "Roll Toggle" 1.0 0.0 1.0 1.0
bool roll = bool(global.godot_roll);
#pragma parameter godot_roll_speed "Roll Speed" 8.0 -20.0 20.0 0.1
float roll_speed = global.godot_roll_speed / 100.; // Positive values are down, negative are up
#pragma parameter godot_roll_size "Roll Size" 15.0 0.0 100.0 1.0
float roll_size = global.godot_roll_size / 8.;
#pragma parameter godot_roll_variation "Roll Variation" 1.8 0.1 5.0 0.1
float roll_variation = global.godot_roll_variation; // This valie is not an exact science. You have to play around with the value to find a look you like. How this works is explained in the code below.
#pragma parameter godot_distort_intensity "Distortion Intensity" 0.05 0.0 0.2 0.01
float distort_intensity = global.godot_distort_intensity; // The distortion created by the rolling effect.

#pragma parameter godot_noise_opacity "Noise Opacity" 0.4 0.0 1.0 0.01
float noise_opacity = global.godot_noise_opacity;
#pragma parameter godot_noise_speed "Noise Speed" 5.0 0.0 20.0 0.5
float noise_speed = global.godot_noise_speed; // There is a movement in the noise pattern that can be hard to see first. This sets the speed of that movement.

#pragma parameter godot_static_noise_intensity "Static Noise Intensity" 0.06 0.0 1.0 0.01
float static_noise_intensity = global.godot_static_noise_intensity;

#pragma parameter godot_aberration "Aberration" 0.03 -1.0 1.0 0.01
float aberration = global.godot_aberration / 2.; // Chromatic aberration, a distortion on each color channel.
#pragma parameter godot_brightness "Brightness" 1.4 0.0 2.0 0.01
float brightness = global.godot_brightness; // When adding scanline gaps and grille the image can get very dark. Brightness tries to compensate for that.
#pragma parameter godot_discolor "Discolor Toggle" 0.0 0.0 1.0 1.0
bool discolor = bool(global.godot_discolor); // Add a discolor effect simulating a VHS

#pragma parameter godot_warp_amount "Warp Amount" 1.0 0.0 5.0 0.1
float warp_amount = global.godot_warp_amount; // Warp the texture edges simulating the curved glass of a CRT monitor or old TV.
#pragma parameter godot_clip_warp "Clip Warp Toggle" 0.0 0.0 1.0 1.0
bool clip_warp = bool(global.godot_clip_warp);

#pragma parameter godot_vignette_intensity "Vignette Intensity" 0.4 0.0 1.0 0.01
float vignette_intensity = global.godot_vignette_intensity; // Size of the vignette, how far towards the middle it should go.
#pragma parameter godot_vignette_opacity "Vignette Opacity" 0.5 0.0 1.0 0.01
float vignette_opacity = global.godot_vignette_opacity;

#pragma parameter godot_moire "Warp Scanlines/Mask (Moire!)" 0.0 0.0 1.0 1.0
bool moire = bool(global.godot_moire);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// compatibility macros
#define COLOR FragColor
#define SCREEN_TEXTURE Source
#define TIME float(params.FrameCount)
#define SCREEN_UV (vTexCoord.xy * params.OutputSize.xy)
#define UV vTexCoord.xy

// Used by the noise functin to generate a pseudo random value between 0.0 and 1.0
vec2 random(vec2 uv){
    uv = vec2( dot(uv, vec2(127.1,311.7) ),
               dot(uv, vec2(269.5,183.3) ) );
    return -1.0 + 2.0 * fract(sin(uv) * 43758.5453123);
}

// Generate a Perlin noise used by the distortion effects
float noise(vec2 uv) {
    vec2 uv_index = floor(uv);
    vec2 uv_fract = fract(uv);

    vec2 blur = smoothstep(0.0, 1.0, uv_fract);

    return mix( mix( dot( random(uv_index + vec2(0.0,0.0) ), uv_fract - vec2(0.0,0.0) ),
                     dot( random(uv_index + vec2(1.0,0.0) ), uv_fract - vec2(1.0,0.0) ), blur.x),
                mix( dot( random(uv_index + vec2(0.0,1.0) ), uv_fract - vec2(0.0,1.0) ),
                     dot( random(uv_index + vec2(1.0,1.0) ), uv_fract - vec2(1.0,1.0) ), blur.x), blur.y) * 0.5 + 0.5;
}

// Takes in the UV and warps the edges, creating the spherized effect
vec2 warp(vec2 uv){
	vec2 delta = uv - 0.5;
	float delta2 = dot(delta.xy, delta.xy);
	float delta4 = delta2 * delta2;
	float delta_offset = delta4 * warp_amount;
	
	return uv + delta * delta_offset;
}

// Adds a black border to hide stretched pixel created by the warp effect
float border (vec2 uv){
	float radius = min(warp_amount, 0.08);
	radius = max(min(min(abs(radius * 2.0), abs(1.0)), abs(1.0)), 1e-5);
	vec2 abs_uv = abs(uv * 2.0 - 1.0) - vec2(1.0, 1.0) + radius;
	float dist = length(max(vec2(0.0), abs_uv)) / radius;
	float square = smoothstep(0.96, 1.0, dist);
	return clamp(1.0 - square, 0.0, 1.0);
}

// Adds a vignette shadow to the edges of the image
float vignette(vec2 uv){
	uv *= 1.0 - uv.xy;
	float vignette = uv.x * uv.y * 15.0;
	return pow(vignette, vignette_intensity * vignette_opacity);
}

void main()
{
	vec2 uv = overlay ? warp(SCREEN_UV) : warp(UV); // Warp the uv. uv will be used in most cases instead of UV to keep the warping
	vec2 text_uv = uv;
	vec2 roll_uv = vec2(0.0);
	float time = roll ? TIME : 0.0;
	

	// Pixelate the texture based on the given resolution.
	if (pixelate)
	{
		text_uv = ceil(uv * resolution) / resolution;
	}
	
	// Create the rolling effect. We need roll_line a bit later to make the noise effect.
	// That is why this runs if roll is true OR noise_opacity is over 0.
	float roll_line = 0.0;
	if ((params.FrameDirection < 0.0 && roll))
	{
		// Create the areas/lines where the texture will be distorted.
		roll_line = smoothstep(0.3, 0.5, sin(uv.y * (roll_size*global.godot_roll * 10.) - (time * roll_speed / 50.) ) );
		// Create more lines of a different size and apply to the first set of lines. This creates a bit of variation.
		roll_line *= roll_line * smoothstep(0.3, 0.4, sin(uv.y * (roll_size*global.godot_roll) * roll_variation - (0.1*time * roll_speed / 12. * roll_variation) ) );
		// Distort the UV where where the lines are
		roll_uv = vec2(( roll_line * distort_intensity * (1.-UV.x)), 0.0);
	}
	
	vec4 text;
	if (roll)
	{
		// If roll is true distort the texture with roll_uv. The texture is split up into RGB to 
		// make some chromatic aberration. We apply the aberration to the red and green channels accorging to the aberration parameter
		// and intensify it a bit in the roll distortion.
		text.r = texture(SCREEN_TEXTURE, text_uv + roll_uv * 0.8 + vec2(aberration, 0.0) * .1).r;
		text.g = texture(SCREEN_TEXTURE, text_uv + roll_uv * 1.2 - vec2(aberration, 0.0) * .1 ).g;
		text.b = texture(SCREEN_TEXTURE, text_uv + roll_uv).b;
		text.a = 1.0;
	}
	else
	{
		// If roll is false only apply the aberration without any distorion. The aberration values are very small so the .1 is only 
		// to make the slider in the Inspector less sensitive.
		text.r = texture(SCREEN_TEXTURE, text_uv + vec2(aberration, 0.0) * .1).r;
		text.g = texture(SCREEN_TEXTURE, text_uv - vec2(aberration, 0.0) * .1).g;
		text.b = texture(SCREEN_TEXTURE, text_uv).b;
		text.a = 1.0;
	}
	
	float r = text.r;
	float g = text.g;
	float b = text.b;
	
	// don't warp the scanlines and mask unless you want moire
	uv = (moire) ? warp(UV) : UV;
	
	// CRT monitors don't have pixels but groups of red, green and blue dots or lines, called grille. We isolate the texture's color channels 
	// and divide it up in 3 offsetted lines to show the red, green and blue colors next to each other, with a small black gap between.
	if (grille_opacity > 0.0){
		
		float g_r = smoothstep(0.85, 0.95, abs(sin(uv.x * (resolution.x * 3.14159265))));
		r = mix(r, r * g_r, grille_opacity);
		
		float g_g = smoothstep(0.85, 0.95, abs(sin(1.05 + uv.x * (resolution.x * 3.14159265))));
		g = mix(g, g * g_g, grille_opacity);
		
		float b_b = smoothstep(0.85, 0.95, abs(sin(2.1 + uv.x * (resolution.x * 3.14159265))));
		b = mix(b, b * b_b, grille_opacity);
		
	}

	// Apply the grille to the texture's color channels and apply Brightness. Since the grille and the scanlines (below) make the image very dark you
	// can compensate by increasing the brightness.
	text.r = clamp(r * brightness, 0.0, 1.0);
	text.g = clamp(g * brightness, 0.0, 1.0);
	text.b = clamp(b * brightness, 0.0, 1.0);
	
	// Scanlines are the horizontal lines that make up the image on a CRT monitor. 
	// Here we are actual setting the black gap between each line, which I guess is not the right definition of the word, but you get the idea  
	float scanlines = 0.5;
	if (scanlines_opacity > 0.0)
	{
		// Same technique as above, create lines with sine and applying it to the texture. Smoothstep to allow setting the line size.
		scanlines = smoothstep(scanlines_width, scanlines_width + 0.5, abs(sin(uv.y * (resolution.y * 3.14159265))));
		text.rgb = mix(text.rgb, text.rgb * vec3(scanlines), scanlines_opacity);
	}
	uv = warp(UV);	
	// Apply the banded noise.
	if (roll || params.FrameDirection < 0.0)
	{
		// Generate a noise pattern that is very stretched horizontally, and animate it with noise_speed
		float noise = smoothstep(0.4, 0.5, noise(uv * vec2(2.0, 200.0) + vec2(10.0, (TIME * (noise_speed))) ) );
		
		// We use roll_line (set above) to define how big the noise should be vertically (multiplying cuts off all black parts).
		// We also add in some basic noise with random() to break up the noise pattern above. The noise is sized according to 
		// the resolution value set in the inspector. If you don't like this look you can 
		// change "ceil(uv * resolution) / resolution" to only "uv" to make it less pixelated. Or multiply resolution with som value
		// greater than 1.0 to make them smaller.
		roll_line *= noise * scanlines * clamp(random((ceil(uv * resolution) / resolution) + vec2(TIME * 0.8, 0.0)).x + 0.8, 0.0, 1.0);
		// Add it to the texture based on noise_opacity
		text.rgb = clamp(mix(text.rgb, text.rgb + roll_line, noise_opacity), vec3(0.0), vec3(1.0));
	}
	
	// Apply static noise by generating it over the whole screen in the same way as above
	if (static_noise_intensity > 0.0)
	{
		text.rgb += clamp(random((ceil(uv * resolution) / resolution) + fract(TIME / 100)).x, 0.0, 1.0) * static_noise_intensity;
	}
	
	// Apply a black border to hide imperfections caused by the warping.
	// Also apply the vignette
	text.rgb *= border(uv);
	text.rgb *= vignette(uv);
	// Hides the black border and make that area transparent. Good if you want to add the the texture on top an image of a TV or monitor.
	if (clip_warp)
	{
		text.a = border(uv);
	}
	
	// Apply discoloration to get a VHS look (lower saturation and higher contrast)
	// You can play with the values below or expose them in the Inspector.
	float saturation = 0.5;
	float contrast = 1.2;
	if (discolor)
	{
		// Saturation
		vec3 greyscale = vec3(text.r + text.g + text.b) / 3.;
		text.rgb = mix(text.rgb, greyscale, saturation);
		
		// Contrast
		float midpoint = pow(0.5, 2.2);
		text.rgb = (text.rgb - vec3(midpoint)) * contrast + midpoint;
	}
	
	COLOR = text;
}
