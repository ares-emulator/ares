//textures and samplers
#ifndef screenLinesRes
    #define screenLinesRes 480.0    //Screen Resolution (to use in _TapeTex, has to be the same as screenLinesNum)
#endif

#ifndef noiseLinesRes
    #define noiseLinesRes 480.0    //Vertical Resolution (to use in _TapeTex, has to be the same as noiseLinesNum)
#endif

const float fisheyeSize = 1.2;                     //Size (DO NOT CHANGE)
const float filmGrainPower = 1.0;                //Film Grain Power (DO NOT CHANGE)
const float feedbackAmp = 1.0;                     //Feedback Amp (DO NOT CHANGE)

#define _ScreenParams vec2(params.OutputSize.x, params.OutputSize.y)
#undef PixelSize
#define PixelSize      vec2(params.OutputSize.z, params.OutputSize.w)

const float Pi2 = 6.283185307;

#define TEXHEIGHT   int(noiseLinesRes)
#define TEXWIDTH     int((float(TEXHEIGHT)*float(params.OutputSize.x)/float(params.OutputSize.y)))