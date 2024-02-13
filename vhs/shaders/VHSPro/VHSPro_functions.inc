//functions

#define PI 3.14159265359
const vec3 MOD3 = vec3(443.8975, 397.2973, 491.1871);

float fmod(float a, float b) {
    const float c = fract(abs(a / b)) * abs(b);
    if (a < 0)
        return -c;
    else
        return c;
}

vec3 bms(vec3 c1, vec3 c2){ return 1.0- (1.0-c1)*(1.0-c2); }

float bms(float c1, float c2){ return 1.0- (1.0-c1)*(1.0-c2); }

//turns sth on and off //a - how often 
float onOff(float a, float b, float c, float t)
{
    return step(c, sin(t + a*cos(t*b)));
}

float hash( float n ){ return fract(sin(n)*43758.5453123); }

float hash12(vec2 p){
    vec3 p3  = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract(p3.x * p3.z * p3.y);
}

vec2 hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3.zxy, p3.yzx+19.19);
    return fract(vec2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

//random hash                
vec4 hash42(vec2 p)
{                    
    vec4 p4 = fract(vec4(p.xyxy) * vec4(443.8975,397.2973, 491.1871, 470.7827));
    p4 += dot(p4.wzxy, p4 + 19.19);
    return fract(vec4(p4.x * p4.y, p4.x*p4.z, p4.y*p4.w, p4.x*p4.w));
}

float niq( in vec3 x ){
    const vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    const float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(        mix( hash(n+  0.0), hash(n+  1.0),f.x),
                            mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
                            mix( mix( hash(n+113.0), hash(n+114.0),f.x),
                            mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

float filmGrain(vec2 uv, float t, float c )
{                     
//cheap noise - is ok atm
    return pow(hash12( uv + 0.07*fract( t ) ), 3);
}

vec2 n4rand_bw( vec2 p, float t, float c )
{                
    t = fract( t );//that's why its sort of twitching 
    vec2 nrnd0 = hash22( p + 0.07*t );
    c = 1.0 / (10.0*c); //iMouse.y  / iResolution.y
    return pow(nrnd0, vec2(c,c)); //TODO try to invert 1-...
}

float scanLines(vec2 p, float t)
{
                        
    //cheap (maybe make an option later)
    // float scanLineWidth = 0.26;
    // float scans = 0.5*(cos((p.y*screenLinesNum+t+.5)*2.0*PI) + 1.0);
    // if(scans>scanLineWidth) scans = 1.; else scans = 0.;                        

        float t_sl = 0.0;                           
        //if lines aren't floating -> scanlines also shudn't 
        if (VHS_LinesFloat) {
            t_sl = t*linesFloatSpeed;
        }
                        
        //expensive but better                        
        float scans = 0.5*(cos( (p.y*(screenLinesNum * 0.5)+t_sl)*2.0*PI) + 1.0);
        scans = pow(scans, scanLineWidth); 
        return 1.0 - scans; 
}        

float gcos(vec2 uv, float s, float p)
{
    return (cos( uv.y * PI * 2.0 * s + p)+1.0)*0.5;
}    

//mw - maximum width
//wcs = widthChangeSpeed
//lfs = line float speed = .5
//lf phase = line float phase = .0
vec2 stretch(vec2 uv, float t, float mw, float wcs, float lfs, float lfp){    
   
    const float SLN = screenLinesNum; //TODO use only SLN
    //width change
    const float tt = t*wcs; //widthChangeSpeed
    const float t2 = tt-fmod(tt, 0.5);
                       
    //float dw  = hash42( vec2(0.01, t2) ).x ; //on t and not on y
    float w = gcos(uv, 2.0*(1.0-fract(t2)), PI-t2) * clamp( gcos(uv, fract(t2), t2) , 0.5, 1.0);
    //w = clamp(w,0.,1.);
    w = floor(w*mw)/mw;
    w *= mw;
    //get descreete line number
    float ln = (1.0-fract(t*lfs + lfp)) *screenLinesNum; 
    ln = ln - fract(ln); 
    // float ln = (1.-fmod(t*lfs + lfp, 1.))*SLN; 
    // ln = ln - fmod(ln, 1.); //descreete line number
                       
    //ln = 10.;
    //w = 4.;
                       
    //////stretching part///////
                       
    const float oy = 1.0/SLN; //TODO global
    const float sh2 =  1.0 - fmod(ln, w)/w; // shift 0..1

    // #if VHS_LINESFLOAT_ON
    //     float sh = fmod(t, 1.);                       
    //      uv.y = floor( uv.y *SLN  +sh )/SLN - sh/SLN;
    // #else 
    //      uv.y = floor( uv.y *SLN  )/SLN;
    //  #endif

    // uv.y = floor( uv.y  *SLN  )/SLN ;
                        
    const float slb = SLN / w; //screen lines big        

    //TODO finish
    // #if VHS_LINESFLOAT_ON

        //  if(uv.y<oy*ln && uv.y>oy*(ln-w)) ////if(uv.y>oy*ln && uv.y<oy*(ln+w)) 
        //     uv.y = floor( uv.y*slb +sh2 +sh )/slb - (sh2-1.)/slb - sh/slb;

    //   #else
                           
        if(uv.y<oy*ln && uv.y>oy*(ln-w)) ////if(uv.y>oy*ln && uv.y<oy*(ln+w)) 
            uv.y = floor( uv.y*slb +sh2 )/slb - (sh2-1.0)/slb ;
                      
    // #endif

    return uv;
}

float rnd_rd(vec2 co)
{
    return fract(sin(fmod(dot(co.xy ,vec2(12.9898,78.233)),3.14)) * 43758.5453);
}

//DANG WINDOWS
vec3 rgb2yiq(vec3 c)
{   
    return vec3(
    0.2989*c.x + 0.5959*c.y + 0.2115*c.z,
    0.5870*c.x - 0.2744*c.y - 0.5229*c.z,
    0.1140*c.x - 0.3216*c.y + 0.3114*c.z);
}

vec3 yiq2rgb(vec3 c)
{                
    return vec3(
    1.0*c.x +1.0*c.y +1.0*c.z,
    0.956*c.x - 0.2720*c.y - 1.1060*c.z,
    0.6210*c.x - 0.6474*c.y + 1.7046*c.z);
}

//rgb distortion
vec3 rgbDistortion(vec2 uv,  float magnitude, float t)
{
    magnitude *= 0.0001; // float magnitude = 0.0009;
    vec3 offsetX = vec3( uv.x, uv.x, uv.x );    
    offsetX.r += rnd_rd(vec2(t*0.03,uv.y*0.42)) * 0.001 + sin(rnd_rd(vec2(t*0.2, uv.y)))*magnitude;
    offsetX.g += rnd_rd(vec2(t*0.004,uv.y*0.002)) * 0.004 + sin(t*9.0)*magnitude;
    // offsetX.b = uv.y + rnd_rd(vec2(cos(t*0.01),sin(uv.y)))*magnitude;
    // offsetX.b = uv.y + rand_rd(vec2(cos(t*0.01),sin(uv.y)))*magnitude;
                        
    vec3 col = vec3(0.0, 0.0, 0.0);
    //it cud be optimized / but hm
    col.x = rgb2yiq( texture( SamplerColorVHS, vec2(offsetX.r, uv.y) ).rgb ).x;
    col.y = rgb2yiq( texture( SamplerColorVHS, vec2(offsetX.g, uv.y) ).rgb ).y;
    col.z = rgb2yiq( texture( SamplerColorVHS, vec2(offsetX.b, uv.y) ).rgb ).z;

    col = yiq2rgb(col);
    return col;
}

float rndln(vec2 p, float t)
{
    float sample_ln = rnd_rd(vec2(1.0,2.0*cos(t))*t*8.0 + p*1.0).x;
    sample_ln *= sample_ln;//*sample;
    return sample_ln;
}

float lineNoise(vec2 p, float t)
{                   
    float n = rndln(p* vec2(0.5,1.0) + vec2(1.0,3.0), t)*20.0;
                        
    float freq = abs(sin(t));  //1.
    float c = n*smoothstep(fmod(p.y*4.0 + t/2.0+sin(t + sin(t*0.63)),freq), 0.0,0.95);

    return c;
}


// 3d noise function (iq's)
float n( in vec3 x )
{
    const vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    const float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+0.0), hash(n+1.0),f.x),
    mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
    mix( mix( hash(n+113.0), hash(n+114.0),f.x),
    mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}

float tapeNoiseLines(vec2 p, float t){

//so atm line noise is depending on hash for int values
//i gotta rewrite to for hash for 0..1 values 
//then i can use normilized p for generating lines

    const float y = p.y*_ScreenParams.y;
    const float s = t*2.0;
    return      (niq( vec3(y*0.01 +s,             1.0, 1.0) ) + 0.0)
                *(niq( vec3(y*0.011+1000.0+s,    1.0, 1.0) ) + 0.0) 
                *(niq( vec3(y*0.51+421.0+s,     1.0, 1.0) ) + 0.0)   
                ;
}


float tapeNoise(float nl, vec2 p, float t){

    //TODO custom adjustable density (Probability distribution)
    // but will be expensive (atm its ok)

    //atm its just contrast noise 
                   
    //this generates noise mask
    float nm =     hash12( fract(p+t*vec2(0.234,0.637)) ) 
                // *hash12( fract(p+t*vec2(0.123,0.867)) ) 
                // *hash12( fract(p+t*vec2(0.441,0.23)) );
                                    ;                        
        nm = pow(nm, 4) +0.3; //cheap and ok
        //nm += 0.3 ; //just bit brighter or just more to threshold?

        nl*= nm; // put mask
        // nl += 0.3; //Value add .3//

    if(nl<tapeNoiseTH) nl = 0.0; else nl =1.0;  //threshold
    return nl;
}

vec2 twitchVertical(float freq, vec2 uv, float t){

    float vShift = 0.4*onOff(freq,3.0,0.9, t);
    vShift*=(sin(t)*sin(t*20.0) + (0.5 + 0.1*sin(t*200.0)*cos(t)));
    uv.y = fmod(uv.y + vShift, 1.0); 
    return uv;
}

vec2 twitchHorizonal(float freq, vec2 uv, float t){

    uv.x += sin(uv.y*10.0 + t)/250.0*onOff(freq,4.0,0.3, t)*(1.0+cos(t*80.0))*(1.0/(1.0+20.0*(uv.y-fmod(t/4.0,1.0))*(uv.y-fmod(t/4.0, 1.0))));
    return uv;
}


//all that shit is for postVHS"Pro"_First - end

//all that shit is for postVHS"Pro"_Second

//size 1.2, bend 2.
vec2 fishEye(vec2 uv, float size, float bend)
{
    if (!VHS_FishEye_Hyperspace){
        uv -= vec2(0.5,0.5);
        uv *= size*(1.0/size+bend*uv.x*uv.x*uv.y*uv.y);
        uv += vec2(0.5,0.5);
    }
                        
            if (VHS_FishEye_Hyperspace){

                //http://paulbourke.net/miscellaneous/lenscorrection/

                const float prop = _ScreenParams.x / _ScreenParams.y;
                const vec2 m = vec2(0.5, 0.5 / prop);    
                const vec2 d = (uv*_ScreenParams.xy) /_ScreenParams.x - m;    
                const float r = sqrt(dot(d, d));
                float bind;

                float power = ( 2.0 * 3.141592 / (2.0 * sqrt(dot(m, m))) ) *
                (bend/50.0 - 0.5); //amount of effect

                if (power > 0.0) bind = sqrt(dot(m, m));//stick to corners
                else {if (prop < 1.0) bind = m.x; else bind = m.x;}//stick to borders

                if (power > 0.0) //fisheye
                    uv = m + normalize(d) * tan(r * power) * bind / tan( bind * power);
                else if (power < 0.0) //antifisheye
                    uv = m + normalize(d) * atan(r * -power * 10.0) * bind / atan(-power * bind * 10.0);
                else uv = (uv*_ScreenParams.xy) /_ScreenParams.x; 

            uv.y *=  prop;
        }

    //adjust size
    // uv -= vec2(0.5,0.5);
    // uv *= size;
    // uv += vec2(0.5,0.5);

    return uv;
}

//pulse vignette
float vignette(vec2 uv, float t)
{
    const float vigAmt = 2.5+0.1*sin(t + 5.0*cos(t*5.0));
    float c = (1.0-vigAmt*(uv.y-0.5)*(uv.y-0.5))*(1.0-vigAmt*(uv.x-0.5)*(uv.x-0.5));
    c = pow(abs(c), vignetteAmount); //expensive!
    return c;
}

vec3 t2d(vec2 p)
{
    return rgb2yiq( texture (SamplerColorVHS, p ).rgb );
}

vec3 yiqDist(vec2 uv, float m, float t)
{    
                        m *= 0.0001; // float m = 0.0009;
                        vec3 offsetX = vec3( uv.x, uv.x, uv.x );    

                        offsetX.r += rnd_rd(vec2(t*0.03, uv.y*0.42)) * 0.001 + sin(rnd_rd(vec2(t*0.2, uv.y)))*m;
                        offsetX.g += rnd_rd(vec2(t*0.004,uv.y*0.002)) * 0.004 + sin(t*9.0)*m;
                        // offsetX.b = uv.y + rnd_rd(vec2(cos(t*0.01),sin(uv.y)))*m;
                        // offsetX.b = uv.y + rand_rd(vec2(cos(t*0.01),sin(uv.y)))*m;
                        
                       vec3 signal = vec3(0.0, 0.0, 0.0);
                       //it cud be optimized / but hm
                       signal.x = rgb2yiq( texture( SamplerColorVHS, vec2(offsetX.r, uv.y) ).rgb ).x;
                       signal.y = rgb2yiq( texture( SamplerColorVHS, vec2(offsetX.g, uv.y) ).rgb ).y;
                       signal.z = rgb2yiq( texture( SamplerColorVHS, vec2(offsetX.b, uv.y) ).rgb ).z;

                       // signal = yiq2rgb(col);
                        return signal;    
}

#define fixCoord (p - vec2( 0.5 * PixelSize.x, .0)) 
#define fetch_offset(offset, one_x) t2d(fixCoord + vec2( (offset) * (ONE_X), 0.0));

vec3 bm_screen(vec3 a, vec3 b){     return 1.0- (1.0-a)*(1.0-b); }