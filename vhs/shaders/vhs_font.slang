#version 450

// Text with VHS font
// by Astherix
// https://www.shadertoy.com/view/NdGyDK
// adapted for slang by hunterk

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	int FrameDirection;
	float godot_warp_amount;
} params;

#pragma parameter godot_warp_amount "Warp Amount" 1.0 0.0 5.0 0.1
float warp_amount = params.godot_warp_amount; // Warp the texture edges simulating the curved glass of a CRT monitor or old TV.

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// shadertoy compatibility macros
#define iChannel0 Source
#define iResolution params.OutputSize.xy
#define fragCoord (vTexCoord.xy * params.OutputSize.xy)
#define fragColor FragColor
#define iFrame params.FrameCount
#define iTime (params.FrameCount / 60.)

// Font data gotten from STV5730's ROM, modified for size

/* stv5730.bin (6/18/2022 2:37:22 PM)
   StartOffset(h): 00000000, EndOffset(h): 00000A1F, Length(h): 00000A20 */

int[] font = int[](
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60606060, 0x60606060, 0x60606060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x06000600, 0x1E001E00,
	0x06000600, 0x06000600, 0x06000600,
	0x06000600, 0x1F801F80, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x006000E0, 0x1FC03F80, 0x70006000,
	0x60006000, 0x7FE07FE0, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x006000E0, 0x07C007C0, 0x00E00060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x07800F80, 0x1D803980,
	0x71806180, 0x61806180, 0x7FE07FE0,
	0x01800180, 0x01800180, 0x00000000,
	0x00000000, 0x7FE07FE0, 0x60006000,
	0x7F807FC0, 0x00E00060, 0x00600060,
	0x606070E0, 0x3FC01F00, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60006000, 0x7F807FC0, 0x60E06060,
	0x606070C0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x7FE07FE0, 0x006000C0,
	0x01800300, 0x06000600, 0x06000600,
	0x06000600, 0x06000600, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x606070E0, 0x3FC03FC0, 0x70E06060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60607060, 0x3FE01FE0, 0x00600060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x3FC03FC0, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x06000F00, 0x1F8039C0,
	0x70E06060, 0x60606060, 0x7FE07FE0,
	0x60606060, 0x60606060, 0x00000000,
	0x00000000, 0x7F807FC0, 0x60E06060,
	0x606060E0, 0x7FC07FC0, 0x60E06060,
	0x606060E0, 0x7FC07F80, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60006000, 0x60006000, 0x60006000,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x7F807FC0, 0x60E06060,
	0x60606060, 0x60606060, 0x60606060,
	0x606060E0, 0x7FC07F80, 0x00000000,
	0x00000000, 0x7FE07FE0, 0x60006000,
	0x60006000, 0x7F807F80, 0x60006000,
	0x60006000, 0x7FE07FE0, 0x00000000,
	0x00000000, 0x7FE07FE0, 0x60006000,
	0x60006000, 0x7F807F80, 0x60006000,
	0x60006000, 0x60006000, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60006000, 0x63E063E0, 0x60606060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x60606060, 0x60606060,
	0x60606060, 0x7FE07FE0, 0x60606060,
	0x60606060, 0x60606060, 0x00000000,
	0x00000000, 0x1F801F80, 0x06000600,
	0x06000600, 0x06000600, 0x06000600,
	0x06000600, 0x1F801F80, 0x00000000,
	0x00000000, 0x07E007E0, 0x01800180,
	0x01800180, 0x01800180, 0x01800180,
	0x61807380, 0x3F001E00, 0x00000000,
	0x00000000, 0x606060E0, 0x61C06380,
	0x67007E00, 0x7C007C00, 0x6E006700,
	0x638061C0, 0x60E06060, 0x00000000,
	0x00000000, 0x60006000, 0x60006000,
	0x60006000, 0x60006000, 0x60006000,
	0x60006000, 0x7FE07FE0, 0x00000000,
	0x00000000, 0x60606060, 0x70E079E0,
	0x7FE06F60, 0x66606060, 0x60606060,
	0x60606060, 0x60606060, 0x00000000,
	0x00000000, 0x60606060, 0x60607060,
	0x78607C60, 0x6E606760, 0x63E061E0,
	0x60E06060, 0x60606060, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60606060, 0x60606060, 0x60606060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x7F807FC0, 0x60E06060,
	0x606060E0, 0x7FC07F80, 0x60006000,
	0x60006000, 0x60006000, 0x00000000,
	0x00000000, 0x1F803FC0, 0x70E06060,
	0x60606060, 0x60606060, 0x66606760,
	0x63E071C0, 0x3FE01E60, 0x00000000,
	0x00000000, 0x7F807FC0, 0x60E06060,
	0x606060E0, 0x7FC07F80, 0x6E006700,
	0x638061C0, 0x60E06060, 0x00000000,
	0x00000000, 0x1FE03FE0, 0x70006000,
	0x60007000, 0x3F801FC0, 0x00E00060,
	0x006000E0, 0x7FC07F80, 0x00000000,
	0x00000000, 0x7FE07FE0, 0x06000600,
	0x06000600, 0x06000600, 0x06000600,
	0x06000600, 0x06000600, 0x00000000,
	0x00000000, 0x60606060, 0x60606060,
	0x60606060, 0x60606060, 0x60606060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x60606060, 0x60606060,
	0x60606060, 0x60606060, 0x606070E0,
	0x39C01F80, 0x0F000600, 0x00000000,
	0x00000000, 0x60606060, 0x60606060,
	0x60606060, 0x60606660, 0x6F607FE0,
	0x79E070E0, 0x60606060, 0x00000000,
	0x00000000, 0x60606060, 0x606070E0,
	0x39C01F80, 0x0F000F00, 0x1F8039C0,
	0x70E06060, 0x60606060, 0x00000000,
	0x00000000, 0x60606060, 0x60606060,
	0x70E039C0, 0x1F800F00, 0x06000600,
	0x06000600, 0x06000600, 0x00000000,
	0x00000000, 0x7FE07FE0, 0x006000E0,
	0x01C00380, 0x07000E00, 0x1C003800,
	0x70006000, 0x7FE07FE0, 0x00000000,
	0x00000000, 0x00000000, 0x06000600,
	0x00000000, 0x00000000, 0x00000000,
	0x06000600, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x60006000, 0x00000000,
	0x00000000, 0x00000000, 0x006000E0,
	0x01C00380, 0x07000E00, 0x1C003800,
	0x70006000, 0x00000000, 0x00000000,
	0x00000000, 0x06000600, 0x02000400,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F003F80, 0x31C000C0, 0x1FC03FC0,
	0x70C060C0, 0x7FE03F60, 0x00000000,
	0x00000000, 0x60006000, 0x60006000,
	0x6F807FC0, 0x70E06060, 0x60606060,
	0x606060E0, 0x7FC07F80, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F803FC0, 0x70E06000, 0x60006000,
	0x600070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x00C000C0, 0x00C000C0,
	0x1FC03FC0, 0x30C060C0, 0x60C060C0,
	0x60C030C0, 0x3FE01FE0, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F803FC0, 0x70E06060, 0x7FE07FE0,
	0x600070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x07800FC0, 0x0CC00C00,
	0x0C003F00, 0x3F000C00, 0x0C000C00,
	0x0C000C00, 0x0C000C00, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F603FE0, 0x70E06060, 0x70E03FE0,
	0x1F600060, 0x70E03FC0, 0x1F800000,
	0x00000000, 0x60006000, 0x60006000,
	0x67806FC0, 0x78E07060, 0x60606060,
	0x60606060, 0x60606060, 0x00000000,
	0x00000000, 0x06000600, 0x00000000,
	0x0E000600, 0x06000600, 0x06000600,
	0x06000600, 0x06001F80, 0x00000000,
	0x00000000, 0x01800180, 0x00000000,
	0x03800180, 0x01800180, 0x01800180,
	0x01803180, 0x3B801F00, 0x0E000000,
	0x00000000, 0x60006000, 0x60006000,
	0x606060E0, 0x61C06380, 0x67006F00,
	0x7F8079C0, 0x60E06060, 0x00000000,
	0x00000000, 0x0E000600, 0x06000600,
	0x06000600, 0x06000600, 0x06000600,
	0x06000680, 0x07800300, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x6DC076C0, 0x66606660, 0x66606660,
	0x66606660, 0x66606660, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x6F807FC0, 0x70E07060, 0x60606060,
	0x60606060, 0x60606060, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F803FC0, 0x70E06060, 0x60606060,
	0x606070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x6F807FC0, 0x70E06060, 0x606070E0,
	0x7FC06F80, 0x60006000, 0x60000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F603FE0, 0x70E06060, 0x606070E0,
	0x3FE01F60, 0x00600060, 0x00600000,
	0x00000000, 0x00000000, 0x00000000,
	0x6F807FC0, 0x78E07060, 0x60006000,
	0x60006000, 0x60006000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x1F803FC0, 0x70E07000, 0x3F803FC0,
	0x00E070E0, 0x3FC01F80, 0x00000000,
	0x00000000, 0x06000600, 0x06000600,
	0x1F801F80, 0x06000600, 0x06000600,
	0x06000600, 0x07800380, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x60606060, 0x60606060, 0x60606060,
	0x606070E0, 0x3FE01F60, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x60606060, 0x60606060, 0x606070E0,
	0x39C01F80, 0x0F000600, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x60606060, 0x66606660, 0x66606660,
	0x6F607FE0, 0x39C01080, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x606070E0, 0x39C01F80, 0x0F000F00,
	0x1F8039C0, 0x70E06060, 0x00000000,
	0x00000000, 0x00000000, 0x00000000,
	0x60606060, 0x60606060, 0x70603FE0,
	0x1FE00060, 0x00E01FC0, 0x1F800000,
	0x00000000, 0x00000000, 0x00000000,
	0x7FE07FE0, 0x01C00380, 0x07000E00,
	0x1C003800, 0x7FE07FE0, 0x00000000,
	0x00000000, 0x00001000, 0x18001C00,
	0x1E001F00, 0x1F801FC0, 0x1F801F00,
	0x1E001C00, 0x18001000, 0x00000000,
	0x00000000, 0x00008400, 0xC600E700,
	0xF780FFC0, 0xFFE0FFF0, 0xFFE0FFC0,
	0xF780E700, 0xC6008400, 0x00000000,
	0x00000000, 0x00000210, 0x06300E70,
	0x1EF03FF0, 0x7FF0FFF0, 0x7FF03FF0,
	0x1EF00E70, 0x06300210, 0x00000000,
	0x00000000, 0x00007FE0, 0x7FE07FE0,
	0x7FE07FE0, 0x7FE07FE0, 0x7FE07FE0,
	0x7FE07FE0, 0x7FE07FE0, 0x00000000
);

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

// Character values are on the bottom of this document
// https://github.com/codeman38/vcr-fonts/blob/master/sources/stv5730/STV5730a.pdf

int get_char_pixel(int x, int y, int c, bool inv) {
    int base = ((x >> 3) + (y * 2) + (c * 0x24)) >> 2;
    
    int offset = ((y & 0x1) != 0) ? 0 : 16; 
    int value = (font[base] >> offset) & 0xffff;
    
    if (inv) {
        return (value & (1 << (0xf - (x & 0xf)))) == 0 ? 1 : 2;
    } else {
        return (value & (1 << (0xf - (x & 0xf)))) != 0 ? 1 : 0;
    }
}

int print_char(vec2 p, vec2 g, int c, bool inv, bool border) {
    bool box = (p.x >= (g.x * 12.0)) && (p.x < ((g.x + 1.0) * 12.0)) &&
               (p.y >= (g.y * 18.0)) && (p.y < ((g.y + 1.0) * 18.0));
    
    if (!box) return 0;
    
    if (border && !inv) {
        int pixel = get_char_pixel(int(p.x) % 12, int(p.y) % 18, c, inv);
        
        if (pixel != 0) return 1;
        
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                pixel = get_char_pixel((int(p.x) + x) % 12, (int(p.y) + y) % 18, c, false);
           
                if (pixel != 0) return 2;
            }
        }
    }

    return get_char_pixel(int(p.x) % 12, int(p.y) % 18, c, inv);
}

int characters[2];

void number_to_characters(float n) {
    int high = int(floor(n / 10.0)),
        low = int(n) - (high * 10);
    
    characters[0] = high;
    characters[1] = low;
}

// I don't love this, but we have to match the CRT pass' curvature at least somewhat
// Takes in the UV and warps the edges, creating the spherized effect
vec2 warp(vec2 uv){
	vec2 delta = uv - 0.5;
	float delta2 = dot(delta.xy, delta.xy);
	float delta4 = delta2 * delta2;
	float delta_offset = delta4 * warp_amount;
	
	return uv + delta * delta_offset;
}

void main()
{
	vec2 uv = warp(vTexCoord);
    vec2 p = uv.xy * vec2(300.,225.);//vec2(fragCoord.x, iResolution.y - fragCoord.y) / 2.;
    
    vec4 t = texture(iChannel0, vTexCoord);
	float timer = (params.FrameDirection > 0.5) ? float(params.FrameCount) : 0.0;
    
    int acc = 0;
if(params.FrameDirection < -0.5){ // when rewind is detected, show the REWIND message
		// Rewind
		acc += int(print_char(p, vec2(2.0, 1.0) , 0x1d, false, true)); // R
		acc += int(print_char(p, vec2(3.0, 1.0) , 0x10, false, true)); // E
		acc += int(print_char(p, vec2(4.0, 1.0) , 0x22, false, true)); // W
		acc += int(print_char(p, vec2(5.0, 1.0) , 0x14, false, true)); // I
		acc += int(print_char(p, vec2(6.0, 1.0) , 0x19, false, true)); // N
		acc += int(print_char(p, vec2(7.0, 1.0) , 0x0f, false, true)); // D
		acc += int(print_char(p, vec2(9.0, 1.0) , 0x46, false, true)); // <<
	}
	// blink the PLAY message for 1000 frames
	else if((mod(timer, 100.0) < 50.0) && (timer != 0.0) && (timer < 1000.0))
	{  
		// Play
		acc += int(print_char(p, vec2(2.0, 1.0) , 0x1b, false, true)); // P
		acc += int(print_char(p, vec2(3.0, 1.0) , 0x17, false, true)); // L
		acc += int(print_char(p, vec2(4.0, 1.0) , 0x0c, false, true)); // A
		acc += int(print_char(p, vec2(5.0, 1.0) , 0x24, false, true)); // Y
		acc += int(print_char(p, vec2(7.0, 1.0) , 0x44, false, true)); // >
	}

    float s = mod(floor(iTime),        60.0);
    float m = mod(floor(iTime/60.0),   60.0);
    float h = mod(floor(iTime/3600.0), 24.0);

    // Timecode
    acc += int(print_char(p, vec2(2.0, 10.0) , 0x1e, false, true)); // S
    acc += int(print_char(p, vec2(3.0, 10.0) , 0x1b, false, true)); // P

    number_to_characters(h);
    acc += int(print_char(p, vec2(5.0, 10.0) , characters[0], false, true)); // 0
    acc += int(print_char(p, vec2(6.0, 10.0) , characters[1], false, true)); // 0
    acc += int(print_char(p, vec2(7.0, 10.0) , 0x26, false, true)); // :

    number_to_characters(m);
    acc += int(print_char(p, vec2(8.0, 10.0) , characters[0], false, true)); // 0
    acc += int(print_char(p, vec2(9.0, 10.0) , characters[1], false, true)); // 0
    acc += int(print_char(p, vec2(10.0, 10.0), 0x26, false, true)); // :

    number_to_characters(s);
    acc += int(print_char(p, vec2(11.0, 10.0), characters[0], false, true)); // 0
    acc += int(print_char(p, vec2(12.0, 10.0), characters[1], false, true)); // 0
    
    // This pass lets iChannel0 pass through
    if (acc == 0) fragColor = t;
    if (acc == 1) fragColor = vec4(1.0);
    if (acc == 2) fragColor = vec4(0.0, 0.0, 0.0, 1.0);
	
	// only show the OSD when the content first launches or during rewind
	fragColor = ((timer < 1000.0) || (params.FrameDirection < -0.5)) ? fragColor : t;
}
