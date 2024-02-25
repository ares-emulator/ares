#version 450
// The Legend of Zelda, by Henrique Lorenzi!

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

#define RGB(r,g,b) vec4(r/255.,g/255.,b/255.,1)
#define SPR(x,a,b,c,d,e,f,g,h, i,j,k,l,m,n,o,p) (x <= 7. ? SPR_H(a,b,c,d,e,f,g,h) : SPR_H(i,j,k,l,m,n,o,p))
#define SPR_H(a,b,c,d,e,f,g,h) (a+4.0*(b+4.0*(c+4.0*(d+4.0*(e+4.0*(f+4.0*(g+4.0*(h))))))))
#define SELECT(x,i) mod(floor(i/pow(4.0,float(mod(float(x),8.0)))),4.0)

float hash(float x)
{
    return fract(sin(x) * 43758.5453) * 2.0 - 1.0;
}

vec2 hashPos(float x)
{
	return vec2(
		floor(hash(x) * 3.0) * 32.0 + 16.0,
		floor(hash(x * 1.1) * 2.0) * 32.0 + 16.0
	);
}

vec4 fragColor;

void spr_rock(float x, float y)
{
	float c = 0.;
	if (y == 0.) c = (x < 8. ? 592. : 0.); if (y == 1.) c = (x < 8. ? 2388. : 0.);
	if (y == 2.) c = (x < 8. ? 26948. : 165.); if (y == 3.) c = (x < 8. ? 18769. : 597.);
	if (y == 4.) c = (x < 8. ? 18769. : 2645.); if (y == 5.) c = (x < 8. ? 21073. : 2389.);
	if (y == 6.) c = (x < 8. ? 21077. : 10582.); if (y == 7.) c = (x < 8. ? 21077. : 10902.);
	if (y == 8.) c = (x < 8. ? 21076. : 10646.); if (y == 9.) c = (x < 8. ? 21076. : 10650.);
	if (y == 10.) c = (x < 8. ? 21076. : 10650.); if (y == 11.) c = (x < 8. ? 22101. : 10905.);
	if (y == 12.) c = (x < 8. ? 22101. : 9877.); if (y == 13.) c = (x < 8. ? 21845. : 1449.);
	if (y == 14.) c = (x < 8. ? 25940. : 43685.); if (y == 15.) c = (x < 8. ? 43690. : 2730.);
	
	float s = SELECT(x,c);
	if (s == 0.) fragColor = RGB(252.,216.,168.);
	if (s == 1.) fragColor = RGB(192.,56.,0.);
	if (s == 2.) fragColor = RGB(0.,0.,0.);
}

void spr_tree(float x, float y)
{
	float c = 0.;
	if (y == 0.) c = 0.; if (y == 1.) c = (x < 8. ? 37120. : 10.);
	if (y == 2.) c = (x < 8. ? 21776. : 681.); if (y == 3.) c = (x < 8. ? 22096. : 2469.);
	if (y == 4.) c = (x < 8. ? 21844. : 2709.); if (y == 5.) c = (x < 8. ? 25940. : 10853.);
	if (y == 6.) c = (x < 8. ? 21845. : 2725.); if (y == 7.) c = (x < 8. ? 21861. : 10901.);
	if (y == 8.) c = (x < 8. ? 21844. : 9878.); if (y == 9.) c = (x < 8. ? 22101. : 10901.);
	if (y == 10.) c = (x < 8. ? 21845. : 2729.); if (y == 11.) c = (x < 8. ? 21861. : 2661.);
	if (y == 12.) c = (x < 8. ? 38228. : 682.); if (y == 13.) c = (x < 8. ? 4416. : 26.);
	if (y == 14.) c = (x < 8. ? 512. : 43680.); if (y == 15.) c = (x < 8. ? 43648. : 2730.);
	
	float s = SELECT(x,c);
	if (s == 0.) fragColor = RGB(252.,216.,168.);
	if (s == 1.) fragColor = RGB(0.,156.,0.);
	if (s == 2.) fragColor = RGB(0.,0.,0.);
}

void spr_player_down(float f, float x, float y)
{
	float c = 0.;
	if (f == 0.) {
		if (y == 0.) c = (x < 8. ? 21504. : 21.); if (y == 1.) c = (x < 8. ? 21760. : 85.);
		if (y == 2.) c = (x < 8. ? 64800. : 2175.); if (y == 3.) c = (x < 8. ? 65312. : 2303.);
		if (y == 4.) c = (x < 8. ? 39840. : 2790.); if (y == 5.) c = (x < 8. ? 48032. : 2798.);
		if (y == 6.) c = (x < 8. ? 43648. : 3754.); if (y == 7.) c = (x < 8. ? 59712. : 3435.);
		if (y == 8.) c = (x < 8. ? 45052. : 16218.); if (y == 9.) c = (x < 8. ? 32751. : 15957.);
		if (y == 10.) c = (x < 8. ? 61355. : 14999.); if (y == 11.) c = (x < 8. ? 28655. : 11007.);
		if (y == 12.) c = (x < 8. ? 61423. : 2391.); if (y == 13.) c = (x < 8. ? 28671. : 85.);
		if (y == 14.) c = (x < 8. ? 15016. : 252.); if (y == 15.) c = (x < 8. ? 16128. : 0.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(128.,208.,16.);
		if (s == 2.) fragColor = RGB(255.,160.,68.);
		if (s == 3.) fragColor = RGB(228.,92.,16.);
	}
	if (f == 1.) {
		if (y == 0.) c = (x < 8. ? 21504. : 21.); if (y == 1.) c = (x < 8. ? 21760. : 85.);
		if (y == 2.) c = (x < 8. ? 64800. : 2175.); if (y == 3.) c = (x < 8. ? 65312. : 2303.);
		if (y == 4.) c = (x < 8. ? 39840. : 2790.); if (y == 5.) c = (x < 8. ? 48032. : 2798.);
		if (y == 6.) c = (x < 8. ? 43648. : 3754.); if (y == 7.) c = (x < 8. ? 59648. : 3947.);
		if (y == 8.) c = (x < 8. ? 49136. : 2394.); if (y == 9.) c = (x < 8. ? 65468. : 2389.);
		if (y == 10.) c = (x < 8. ? 48812. : 863.); if (y == 11.) c = (x < 8. ? 49084. : 509.);
		if (y == 12.) c = (x < 8. ? 49084. : 351.); if (y == 13.) c = (x < 8. ? 49148. : 213.);
		if (y == 14.) c = (x < 8. ? 10912. : 252.); if (y == 15.) c = (x < 8. ? 0. : 252.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(128.,208.,16.);
		if (s == 2.) fragColor = RGB(255.,160.,68.);
		if (s == 3.) fragColor = RGB(228.,92.,16.);
	}
	if (f == 2.) {
		if (y == 0.) c = 0.; if (y == 1.) c = (x < 8. ? 41280. : 42.);
		if (y == 2.) c = (x < 8. ? 43472. : 170.); if (y == 3.) c = (x < 8. ? 23252. : 677.);
		if (y == 4.) c = (x < 8. ? 22261. : 12949.); if (y == 5.) c = (x < 8. ? 60917. : 15963.);
		if (y == 6.) c = (x < 8. ? 56791. : 3703.); if (y == 7.) c = (x < 8. ? 32348. : 1021.);
		if (y == 8.) c = (x < 8. ? 31344. : 381.); if (y == 9.) c = (x < 8. ? 60096. : 1375.);
		if (y == 10.) c = (x < 8. ? 43264. : 1370.); if (y == 11.) c = (x < 8. ? 26112. : 2389.);
		if (y == 12.) c = (x < 8. ? 23040. : 2646.); if (y == 13.) c = (x < 8. ? 26944. : 6781.);
		if (y == 14.) c = (x < 8. ? 41296. : 22207.); if (y == 15.) c = (x < 8. ? 0. : 21823.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(228.,92.,16.);
		if (s == 2.) fragColor = RGB(128.,208.,16.);
		if (s == 3.) fragColor = RGB(255.,160.,68.);
	}
}

void spr_player_up(float f, float x, float y)
{
	float c = 0.;
	if (f == 0. || f == 1.) {
		if (f == 1.) x = 15. - x;
		
		if (y == 0.) c = (x < 8. ? 21504. : 21.); if (y == 1.) c = (x < 8. ? 21760. : 85.);
		if (y == 2.) c = (x < 8. ? 21792. : 2133.); if (y == 3.) c = (x < 8. ? 21856. : 2389.);
		if (y == 4.) c = (x < 8. ? 21984. : 2901.); if (y == 5.) c = (x < 8. ? 24480. : 2805.);
		if (y == 6.) c = (x < 8. ? 32640. : 765.); if (y == 7.) c = (x < 8. ? 64960. : 895.);
		if (y == 8.) c = (x < 8. ? 22000. : 981.); if (y == 9.) c = (x < 8. ? 22000. : 3029.);
		if (y == 10.) c = (x < 8. ? 22464. : 3029.); if (y == 11.) c = (x < 8. ? 64832. : 2687.);
		if (y == 12.) c = (x < 8. ? 21824. : 341.); if (y == 13.) c = (x < 8. ? 24512. : 213.);
		if (y == 14.) c = (x < 8. ? 16320. : 60.); if (y == 15.) c = (x < 8. ? 3840. : 0.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(128.,208.,16.);
		if (s == 2.) fragColor = RGB(255.,160.,68.);
		if (s == 3.) fragColor = RGB(228.,92.,16.);
	}
	if (f == 2.) {
		if (y == 0.) c = (x < 8. ? 43584. : 2.); if (y == 1.) c = (x < 8. ? 43660. : 10.);
		if (y == 2.) c = (x < 8. ? 43676. : 42.); if (y == 3.) c = (x < 8. ? 43708. : 810.);
		if (y == 4.) c = (x < 8. ? 43636. : 986.); if (y == 5.) c = (x < 8. ? 43380. : 49365.);
		if (y == 6.) c = (x < 8. ? 26004. : 28901.); if (y == 7.) c = (x < 8. ? 22164. : 23897.);
		if (y == 8.) c = (x < 8. ? 43664. : 22362.); if (y == 9.) c = (x < 8. ? 43664. : 21978.);
		if (y == 10.) c = (x < 8. ? 43616. : 5498.); if (y == 11.) c = (x < 8. ? 21924. : 1493.);
		if (y == 12.) c = (x < 8. ? 43685. : 938.); if (y == 13.) c = (x < 8. ? 32789. : 362.);
		if (y == 14.) c = (x < 8. ? 0. : 1360.); if (y == 15.) c = (x < 8. ? 0. : 1360.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(228.,92.,16.);
		if (s == 2.) fragColor = RGB(128.,208.,16.);
		if (s == 3.) fragColor = RGB(255.,160.,68.);
	}
	
}


void spr_player_left(float f, float x, float y)
{
	float c = 0.;
	if (f == 0.) {
		if (y == 0.) c = (x < 8. ? 16384. : 21.); if (y == 1.) c = (x < 8. ? 43520. : 341.);
		if (y == 2.) c = (x < 8. ? 43648. : 5590.); if (y == 3.) c = (x < 8. ? 43520. : 22010.);
		if (y == 4.) c = (x < 8. ? 63240. : 17918.); if (y == 5.) c = (x < 8. ? 64504. : 1726.);
		if (y == 6.) c = (x < 8. ? 65288. : 687.); if (y == 7.) c = (x < 8. ? 65288. : 85.);
		if (y == 8.) c = (x < 8. ? 23224. : 2389.); if (y == 9.) c = (x < 8. ? 22200. : 10879.);
		if (y == 10.) c = (x < 8. ? 22152. : 10943.); if (y == 11.) c = (x < 8. ? 22536. : 10941.);
		if (y == 12.) c = (x < 8. ? 43016. : 1686.); if (y == 13.) c = (x < 8. ? 21504. : 5461.);
		if (y == 14.) c = (x < 8. ? 0. : 170.); if (y == 15.) c = (x < 8. ? 32768. : 170.);
	}
	if (f == 1.) {
		if (y == 0.) c = 0.; if (y == 1.) c = (x < 8. ? 20480. : 5.);
		if (y == 2.) c = (x < 8. ? 27264. : 85.); if (y == 3.) c = (x < 8. ? 43680. : 1397.);
		if (y == 4.) c = (x < 8. ? 43648. : 5502.); if (y == 5.) c = (x < 8. ? 48576. : 4479.);
		if (y == 6.) c = (x < 8. ? 48892. : 431.); if (y == 7.) c = (x < 8. ? 65480. : 171.);
		if (y == 8.) c = (x < 8. ? 32712. : 21.); if (y == 9.) c = (x < 8. ? 65208. : 421.);
		if (y == 10.) c = (x < 8. ? 64952. : 682.); if (y == 11.) c = (x < 8. ? 62856. : 1706.);
		if (y == 12.) c = (x < 8. ? 22024. : 1450.); if (y == 13.) c = (x < 8. ? 43592. : 10581.);
		if (y == 14.) c = (x < 8. ? 21920. : 10837.); if (y == 15.) c = 2688.;
	}
	if (f == 2.) {
		if (y == 0.) c = 0.; if (y == 1.) c = (x < 8. ? 21504. : 1.);
		if (y == 2.) c = (x < 8. ? 23200. : 21.); if (y == 3.) c = (x < 8. ? 27304. : 93.);
		if (y == 4.) c = (x < 8. ? 43680. : 95.); if (y == 5.) c = (x < 8. ? 61296. : 351.);
		if (y == 6.) c = (x < 8. ? 61375. : 1387.); if (y == 7.) c = (x < 8. ? 65520. : 1066.);
		if (y == 8.) c = (x < 8. ? 24572. : 21.); if (y == 9.) c = (x < 8. ? 43772. : 90.);
		if (y == 10.) c = (x < 8. ? 43760. : 106.); if (y == 11.) c = (x < 8. ? 43584. : 362.);
		if (y == 12.) c = (x < 8. ? 38304. : 1370.); if (y == 13.) c = (x < 8. ? 27296. : 10581.);
		if (y == 14.) c = (x < 8. ? 21864. : 10837.); if (y == 15.) c = (x < 8. ? 170. : 2688.);
	}
	
	float s = SELECT(x,c);
	if (s == 1.) fragColor = RGB(128.,208.,16.);
	if (s == 2.) fragColor = RGB(228.,92.,16.);
	if (s == 3.) fragColor = RGB(255.,160.,68.);
}

void spr_player_right(float f, float x, float y)
{
	spr_player_left(f, 15. - x, y);
}

void spr_sword(float f, float tDirX, float tDirY, float x, float y)
{
	if (f < 4. || f > 32.) return;
	
	if (tDirX > 0.) {x = 15. - x;}
	if (tDirY > 0.) {float temp = y; y = x; x = temp;}
	if (tDirY < 0.) {float temp = y; y = x; x = 15. - temp;}
	
	if (f < 5. || f > 28.) {if (x < 10.) {x -= 4.;}}
	
	if (x < 0.) return;
	
	float c = 0.;
	if (y == 0.) c = 0.; if (y == 1.) c = 0.;
	if (y == 2.) c = 0.; if (y == 3.) c = 0.;
	if (y == 4.) c = 0.; if (y == 5.) c = (x < 8. ? 0. : 20480.);
	if (y == 6.) c = (x < 8. ? 0. : 4096.); if (y == 7.) c = (x < 8. ? 43520. : 39594.);
	if (y == 8.) c = (x < 8. ? 43648. : 39594.); if (y == 9.) c = (x < 8. ? 43520. : 39594.);
	if (y == 10.) c = (x < 8. ? 0. : 4096.); if (y == 11.) c = (x < 8. ? 0. : 20480.);
	if (y == 12.) c = 0.; if (y == 13.) c = 0.;
	if (y == 14.) c = 0.; if (y == 15.) c = 0.;
	
	float s = SELECT(x,c);
	if (s == 1.) fragColor = RGB(128.,208.,16.);
	if (s == 2.) fragColor = RGB(228.,92.,16.);

}

void spr_enemy(float f, float tDirX, float tDirY, float x, float y)
{
	
	if (tDirX > 0.) {x = 15. - x;}
	if (tDirY > 0.) {float temp = y; y = x; x = 15. - temp;}
	if (tDirY < 0.) {float temp = y; y = x; x = temp;}
	
	if (y >= 8.) y = 15. - y;
	
	float c = 0.;
	if (f == 0.) {
		if (y == 0.) c = (x < 8. ? 16384. : 4160.); if (y == 1.) c = (x < 8. ? 16384. : 5201.);
		if (y == 2.) c = (x < 8. ? 16640. : 1365.); if (y == 3.) c = (x < 8. ? 21760. : 5466.);
		if (y == 4.) c = (x < 8. ? 42000. : 5610.); if (y == 5.) c = (x < 8. ? 25680. : 30053.);
		if (y == 6.) c = (x < 8. ? 22864. : 22361.); if (y == 7.) c = (x < 8. ? 43344. : 54618.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(224.,80.,0.);
		if (s == 2.) fragColor = RGB(255.,255.,255.);
		if (s == 3.) fragColor = RGB(255.,160.,0.);
	}
	if (f == 1.) {
		if (y == 0.) c = (x < 8. ? 0. : 1040.); if (y == 1.) c = (x < 8. ? 20480. : 1300.);
		if (y == 2.) c = (x < 8. ? 16384. : 1365.); if (y == 3.) c = (x < 8. ? 21760. : 5466.);
		if (y == 4.) c = (x < 8. ? 42241. : 5610.); if (y == 5.) c = (x < 8. ? 25601. : 30053.);
		if (y == 6.) c = (x < 8. ? 22869. : 22361.); if (y == 7.) c = (x < 8. ? 43349. : 54618.);
		
		float s = SELECT(x,c);
		if (s == 1.) fragColor = RGB(224.,80.,0.);
		if (s == 2.) fragColor = RGB(255.,255.,255.);
		if (s == 3.) fragColor = RGB(255.,160.,0.);
	}
	
	if (x >= 8.) x = 15. - x;
	if (y >= 8.) y = 15. - y;
	if (f == 2. || f == 5. || f == 7.) {		
		if (y == 0.) c = 0.; if (y == 1.) c = 0.;
		if (y == 2.) c = 0.; if (y == 3.) c = (x < 8. ? 64. : 0.);
		if (y == 4.) c = (x < 8. ? 32768. : 0.); if (y == 5.) c = (x < 8. ? 17408. : 0.);
		if (y == 6.) c = (x < 8. ? 32768. : 0.); if (y == 7.) c = (x < 8. ? 26112. : 0.);
		
		float s = SELECT(x,c);
		if (mod(floor(iGlobalTime * 10.),2.) == 0.) {
			if (s == 2.) s = 1.;
			else if (s == 1.) s= 2.;
		}
		
		if (s == 1.) fragColor = RGB(255.,255.,255.);
		if (s == 2.) fragColor = RGB(104.,136.,255.);
	}
	if (f == 3. || f == 6.) {
		if (y == 0.) c = 0.; if (y == 1.) c = (x < 8. ? 4. : 0.);
		if (y == 2.) c = (x < 8. ? 32. : 0.); if (y == 3.) c = (x < 8. ? 32832. : 0.);
		if (y == 4.) c = (x < 8. ? 33024. : 0.); if (y == 5.) c = (x < 8. ? 17408. : 0.);
		if (y == 6.) c = (x < 8. ? 40960. : 0.); if (y == 7.) c = (x < 8. ? 26240. : 0.);
		
		float s = SELECT(x,c);
		if (mod(floor(iGlobalTime * 10.),2.) == 0.) {
			if (s == 2.) s = 1.;
			else if (s == 1.) s= 2.;
		}
		
		if (s == 1.) fragColor = RGB(255.,255.,255.);
		if (s == 2.) fragColor = RGB(104.,136.,255.);

	}
}


void background(vec2 p)
{
	float tileX = floor((p.x - 8.0) / 16.0);
	float tileY = floor((p.y - 8.0) / 16.0);
	float pixelX = mod((p.x - 8.0), 16.0);
	float pixelY = 15.0 - mod((p.y - 8.0), 16.0);
	
	if ((tileX >= -4. && tileX <= 2. && tileY >= -2. && tileY <= 0.) ||
		(tileX == -5. && tileY == 1.) ||
		(tileX == -5. && tileY == -3.) ||
		(tileX == 3. && tileY == 1.) ||
		(tileX == 3. && tileY == -3.) ||
		
		(tileX == 5. && tileY == -1.) ||
		(tileX == -1. && tileY == -3.) ||
		(tileX == -7. && tileY == -1.) ||
		((mod(tileX, 2.0) == 0. || mod(tileY, 2.0) == 0.) &&
	   ((tileX >= -6. && tileX <= 4.) || (tileY >= -4. && tileY <= 2.)))) {
			
			if (mod(pixelX * 5.5 + pixelY * 4.,21.) == 0.)
				fragColor = mix(RGB(252.,206.,168.),RGB(252.,196.,118.),length(p / 256.));
			else
				fragColor = RGB(252.,216.,168.);
	} else {
		if (tileX >= -5. && tileX <= 3. && tileY >= -3. && tileY <= 2.)
			spr_tree(pixelX,pixelY);
		else
			spr_rock(pixelX,pixelY);
	}
}

void mainImage( out vec4 oFragColor, in vec2 fragCoord )
{
	float size = 2.;
	if (iResolution.y < 200.) size = 1.;
	if (iResolution.y > 600.) size = 4.;
	vec2 uv = floor((fragCoord.xy - iResolution.xy / 2.0) / size);
	
	
	
	background(uv);
	
	
	float time = floor(iGlobalTime * 60.0);
	
	const float walkTime = 60.0 * 5.0;
	float walkIndex = floor(time / walkTime);
	float walkFrame = mod(time, walkTime);
	
	vec2 lastPlayerPos = hashPos(walkIndex - 1.);
	vec2 curPlayerPos = hashPos(walkIndex);
	vec2 playerPos = lastPlayerPos;
	
	float dirX = 0., dirXRand = 0.;
	float dirY = 0., dirYRand = 0.;
	float dirRand = floor(abs(hash(floor(time / 50.0))) * 4.0);
	if (dirRand == 0.) dirXRand = 1.;
	else if (dirRand == 1.) dirXRand = -1.;
	else if (dirRand == 2.) dirYRand = 1.;
	else dirYRand = -1.;
	
	float frame = floor(mod(time / 6.0,2.0));
	float swordFrame = 0.;
	
	if (hash(walkIndex * 3.84) < 0.) {
		float yDisp = abs(lastPlayerPos.y - curPlayerPos.y);
		float xDisp = abs(lastPlayerPos.x - curPlayerPos.x);
		float ySign = sign(curPlayerPos.y - lastPlayerPos.y);
		float xSign = sign(curPlayerPos.x - lastPlayerPos.x);
		
		if (walkFrame < yDisp) {
			playerPos = vec2(lastPlayerPos.x,lastPlayerPos.y + ySign * walkFrame);
			dirY = ySign;
		} else if (walkFrame < yDisp + xDisp - 16.0) {
			playerPos = vec2(lastPlayerPos.x + xSign * (walkFrame - yDisp),curPlayerPos.y);
			dirX = xSign;
		} else if (walkFrame < yDisp + xDisp - 16.0 + 40.0) {
			dirX = xSign;
			frame = 2.;
			swordFrame = walkFrame - (yDisp + xDisp - 16.0);
			playerPos = vec2(lastPlayerPos.x + xSign * (xDisp - 16.0),curPlayerPos.y);
		} else if (walkFrame < yDisp + xDisp - 16.0 + 56.0) {
			dirX = xSign;
			playerPos = vec2(lastPlayerPos.x + xSign * (walkFrame - (yDisp - 16.0 + 56.0)),curPlayerPos.y);
		} else {
			frame = 0.;
			if (walkFrame < yDisp + xDisp - 16.0 + 90.) dirX = xSign;
			playerPos = curPlayerPos;
		}
	} else {
		float yDisp = abs(lastPlayerPos.y - curPlayerPos.y);
		float xDisp = abs(lastPlayerPos.x - curPlayerPos.x);
		float ySign = sign(curPlayerPos.y - lastPlayerPos.y);
		float xSign = sign(curPlayerPos.x - lastPlayerPos.x);
		
		if (walkFrame < xDisp) {
			dirX = xSign;
			playerPos = vec2(lastPlayerPos.x + xSign * walkFrame,lastPlayerPos.y);
		} else if (walkFrame < yDisp + xDisp - 16.0) {
			dirY = ySign;
			playerPos = vec2(curPlayerPos.x,lastPlayerPos.y + ySign * (walkFrame - xDisp));
		} else if (walkFrame < yDisp + xDisp - 16.0 + 40.0) {
			frame = 2.;
			swordFrame = walkFrame - (yDisp + xDisp - 16.0);
			dirY = ySign;
			playerPos = vec2(curPlayerPos.x,lastPlayerPos.y + ySign * (yDisp - 16.0));
		} else if (walkFrame < yDisp + xDisp - 16.0 + 56.0) {
			dirY = ySign;
			playerPos = vec2(curPlayerPos.x,lastPlayerPos.y + ySign * (walkFrame - (xDisp - 16.0 + 56.0)));
		} else {
			frame = 0.;
			if (walkFrame < yDisp + xDisp - 16.0 + 90.) dirY = ySign;
			playerPos = curPlayerPos;
		}
	}
	
	
	if (dirX == 0. && dirY == 0.) {
		dirY = dirYRand;
		dirX = dirXRand;
		frame = 0.;
	}
	
	const int enemyNum = 3;
	for(int i = 0; i < enemyNum; i++) {
		float wi = walkIndex + float(i);
		vec2 eLastPos = hashPos(wi - 1.);
		vec2 eCurPos = hashPos(wi);
		vec2 ePos = vec2(0,0);
		
		float eDirX = 0.;
		float eDirY = 0.;
		
		float yDisp = abs(eCurPos.y - eLastPos.y);
		float xDisp = abs(eCurPos.x - eLastPos.x);
		float ySign = sign(eCurPos.y - eLastPos.y);
		float xSign = sign(eCurPos.x - eLastPos.x);
		
		float eFrame = floor(mod((time + 2.5) / 6.0,2.0));
		
		if (hash(wi * 3.84) < 0.) {
			if (eLastPos.x != eCurPos.x) {
				float s = (hash(wi) < 0. ? 1. : -1.);
				float y = (xDisp + yDisp - (walkFrame - walkTime * float(i)));
				float yc = abs(hash(wi)) * 64. + 64.;
				
				if (y < 0.) {
					ePos = eCurPos;
					eDirY = s;
					eFrame = 2. + floor(abs(y / 5.));
				} else if (y < yc) {
					ePos = vec2(eCurPos.x, eCurPos.y + s * yc + s * (y - yc));
					eDirY = s;
				} else {
					ePos = vec2(eCurPos.x, eCurPos.y + s * yc - s * (y - yc));
					eDirY = -s;
				}
			}
		} else {
			if (eLastPos.y != eCurPos.y) {
				float s = (hash(wi) < 0. ? 1. : -1.);
				float x = (xDisp + yDisp - (walkFrame - walkTime * float(i)));
				float xc = abs(hash(wi)) * 64. + 64.;
				
				if (x < 0.) {
					ePos = eCurPos;
					eDirX = s;
					eFrame = 2. + floor(abs(x / 5.));
				} else if (x < xc) {
					ePos = vec2(eCurPos.x + s * xc + s * (x - xc), eCurPos.y);
					eDirX = -s;
				} else {
					ePos = vec2(eCurPos.x + s * xc - s * (x - xc), eCurPos.y);
					eDirX = s;
				}
			}
		}
		
		ePos = floor(ePos);
		
		if ((eDirX != 0. || eDirY != 0.) &&
			uv.x >= ePos.x - 8. && uv.x <= ePos.x + 7. &&
			uv.y >= ePos.y - 8. && uv.y <= ePos.y + 7.) {
				float epx = uv.x - ePos.x + 8.;
				float epy = ePos.y - uv.y + 7.;
				spr_enemy(eFrame, eDirX, eDirY, epx, epy);
		}
		
	}
	
	if (frame == 2.) {
		float spx = 0.;
		float spy = 0.;
		if (dirX > 0.) {spx = 15.; spy = -1.;}
		if (dirX < 0.) {spx = -15.; spy = -1.;}
		if (dirY > 0.) {spx = -3.; spy = 15.;}
		if (dirY < 0.) {spx = 1.; spy = -15.;}
		
		if (uv.x >= playerPos.x - 8. + spx && uv.x <= playerPos.x + 7. + spx &&
			uv.y >= playerPos.y - 7. + spy && uv.y <= playerPos.y + 8. + spy) {
			
			float sx = uv.x - playerPos.x + 8. - spx;
			float sy = playerPos.y - uv.y + 8. + spy;
			spr_sword(swordFrame,dirX,dirY,sx,sy);
		}
	}
	
	if (uv.x >= playerPos.x - 8. && uv.x <= playerPos.x + 7. &&
		uv.y >= playerPos.y - 7. && uv.y <= playerPos.y + 8.) {
			float px = uv.x - playerPos.x + 8.;
			float py = playerPos.y - uv.y + 8.;
		
			if (dirX > 0.) spr_player_right(frame,px,py);
			if (dirX < 0.) spr_player_left(frame,px,py);
			if (dirY > 0.) spr_player_up(frame,px,py);
			if (dirY < 0.) spr_player_down(frame,px,py);
	}
	
    oFragColor = fragColor;
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
  FragCoord.y = -FragCoord.y;
  mainImage(FragColor,FragCoord);
}
