#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////
//
//  crt-resswitch-glitch-koko
//  Copyright (C) 2024 Antonio Orefice <kokoko3k@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 3 of the License, or any later version.
//
////////////////////////////////////////////////////////////////////////////////


layout(std140, set = 0, binding = 0) uniform UBO {
    mat4 MVP;
    vec4 SourceSize;
    vec4 OriginalHistorySize1;
    vec4 OriginalHistorySize2;
    vec4 OriginalHistorySize3;
    vec4 OriginalHistorySize4;
    vec4 OriginalHistorySize5;
    float RESSWITCH_ENABLE;
    float RESSWITCH_GLITCH_TRESHOLD;
    float RESSWITCH_GLITCH_BAR_STR;
    float RESSWITCH_GLITCH_BAR_SIZE;
    float RESSWITCH_GLITCH_BAR_SMOOTH;
    float RESSWITCH_GLITCH_SHAKE_MAX;
    float RESSWITCH_GLITCH_ROT_MAX;
    float RESSWITCH_GLITCH_WOB_MAX;
    uint FrameCount;
} global;

#pragma parameter RESSWITCH_ENABLE "★ Resolution switch glitch enable? (no d3d11) ==>"     1.0  0.0 1.0  1.0
#pragma parameter RESSWITCH_GLITCH_TRESHOLD  "    Glitch if vertical resolution changes at least by" 0.1 0.01 1.0 0.01
#pragma parameter RESSWITCH_GLITCH_SHAKE_MAX "    Shake amplitude"       0.25  0.0 1.0 0.01
#pragma parameter RESSWITCH_GLITCH_ROT_MAX   "    Tilt amplitude"        0.2  0.0 1.0 0.01
#pragma parameter RESSWITCH_GLITCH_WOB_MAX     "    Wobblying amplitude"  0.1 0.0 1.0 0.01
#pragma parameter RESSWITCH_GLITCH_BAR_SIZE    "    Bars size"            0.5 0.0 1.0 0.01
#pragma parameter RESSWITCH_GLITCH_BAR_SMOOTH  "    Bars smoothness"      1.0 0.01 1.0 0.01
#pragma parameter RESSWITCH_GLITCH_BAR_STR     "    Bars strength"  0.6 0.0 1.0 0.01


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float vSwitchres_show_bar;
layout(location = 2) out float vSwitchres_bar_size;
layout(location = 3) out float vWobblyAmp;
layout(location = 4) out float vWobPhase;

#define _3kHIST0 global.SourceSize.xy
#define _3kHIST1 global.OriginalHistorySize1.xy
#define _3kHIST2 global.OriginalHistorySize2.xy
#define _3kHIST3 global.OriginalHistorySize3.xy
#define _3kHIST4 global.OriginalHistorySize4.xy
#define _3kHIST5 global.OriginalHistorySize5.xy

bool changed_enough(vec2 current, vec2 past){
    return (abs(current.y - past.y) / current.y > global.RESSWITCH_GLITCH_TRESHOLD) ||
           (abs(current.x - past.x) / current.x > global.RESSWITCH_GLITCH_TRESHOLD);
}

void main() {
    
    float z   = 1.0; // zoom (1.0)
    float sty = 1.0; // Y stretch (1.0)
    float stx = 1.0; // H stretch (1.0)
    float rx  = 0.0; // X rot
    float ry  = 0.0; // Y rot 
    float sx1 = 0.0; // H slope
    float sy1 = 0.0; // V slope
    float tx  = 0.0; // H shift
    float ty  = 0.0; // V shift
    float angleZ = 0.0; // Z rot
    
    
    vTexCoord = TexCoord * 1.0001;

    vWobblyAmp = 0.0;
    
    //SHAKING
        vSwitchres_show_bar = 0.0;
        
        // d3d11 is unable to access OriginalHistorySize#, it returns vec4(0.0)
        // do a sanity check:
        bool skip_glitch = (_3kHIST1.x+_3kHIST1.y) + (_3kHIST2.x+_3kHIST2.y) +
                           (_3kHIST3.x+_3kHIST3.y) + (_3kHIST4.x+_3kHIST4.y) +
                           (_3kHIST5.x+_3kHIST5.y) == 0.0;

        // Skip a bunch of frames at the beginning to prevent unwanted glitches
        // with flycast core and maybe with fast-forward too.
        
        if (global.RESSWITCH_ENABLE > 0.5 && global.FrameCount > 10 && !skip_glitch) {
            vWobPhase = global.FrameCount * 0.01;
            vSwitchres_bar_size = global.RESSWITCH_GLITCH_SHAKE_MAX  * global.RESSWITCH_GLITCH_BAR_SIZE ;
            
            if (changed_enough(_3kHIST0, _3kHIST1)) {
                //1st
                vWobblyAmp = global.RESSWITCH_GLITCH_WOB_MAX * 0.001;
                vSwitchres_show_bar = global.RESSWITCH_GLITCH_BAR_STR;
                vTexCoord.y += global.RESSWITCH_GLITCH_SHAKE_MAX *  0.5 ;
                rx = global.RESSWITCH_GLITCH_ROT_MAX * 1.0 * 0.2;
            }

            else if (changed_enough(_3kHIST0, _3kHIST2)) {
                //2nd frame
                vWobblyAmp = global.RESSWITCH_GLITCH_WOB_MAX * 0.01;
                vSwitchres_show_bar = global.RESSWITCH_GLITCH_BAR_STR;
                vTexCoord.y += global.RESSWITCH_GLITCH_SHAKE_MAX * -1.0;
                rx =  global.RESSWITCH_GLITCH_ROT_MAX *  -1.0 * 0.2;
            }

            else if (changed_enough(_3kHIST0, _3kHIST3)) {
                //3rd frame
                vWobblyAmp = global.RESSWITCH_GLITCH_WOB_MAX * 0.1;
                vSwitchres_show_bar = global.RESSWITCH_GLITCH_BAR_STR;
                vTexCoord.y += global.RESSWITCH_GLITCH_SHAKE_MAX * +0.5;
                rx =  global.RESSWITCH_GLITCH_ROT_MAX * 0.5 * 0.2;
            }

            else if (changed_enough(_3kHIST0, _3kHIST4)) {
                //4th frame
                vWobblyAmp = global.RESSWITCH_GLITCH_WOB_MAX * 0.01;
                vSwitchres_show_bar = global.RESSWITCH_GLITCH_BAR_STR;
                vTexCoord.y += global.RESSWITCH_GLITCH_SHAKE_MAX * -0.5;
                rx =  global.RESSWITCH_GLITCH_ROT_MAX * - 0.5 * 0.2;
            }

            else if (changed_enough(_3kHIST0, _3kHIST5)) {
                //5th last frame
                vWobblyAmp = global.RESSWITCH_GLITCH_WOB_MAX * 0.001;
                vSwitchres_show_bar = global.RESSWITCH_GLITCH_BAR_STR;
                vTexCoord.y += global.RESSWITCH_GLITCH_SHAKE_MAX * +0.25;
                rx =  global.RESSWITCH_GLITCH_ROT_MAX * 0.25 * 0.2;
            }
        }
        
        //Maybe for future ues
        mat4 projectionMatrix = mat4( 
            stx, sx1, 0.0,  tx,
            sy1, sty, 0.0,  ty,
            0.0, 0.0, -1.0, 0.0,
            -ry, rx,  0.0,  z );
        
        //Rotate around Z axis (angleZ set before)?
        mat4 m4rotationZ = mat4(
            cos(angleZ), -sin(angleZ), 0.0, 0.0,
            sin(angleZ), cos(angleZ), 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0 );

        gl_Position = global.MVP * Position * projectionMatrix * m4rotationZ;

}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float vSwitchres_show_bar;
layout(location = 2) in float vSwitchres_bar_size;
layout(location = 3) in float vWobblyAmp;
layout(location = 4) in float vWobPhase;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D Source;

#define _3kPI 3.141592653589793


void main() {
    vec2 co = vTexCoord;
    // BORDER BARS
        float switchresglitch = 0.0;  
        float sin_formula1 =   sin(_3kPI*(co.y-1.0)/vSwitchres_bar_size);
        float sin_formula2 =  -sin(_3kPI*co.y/vSwitchres_bar_size);
        if (co.y > 1.0) {
            if (co.y < 1.0 + vSwitchres_bar_size)
                switchresglitch = pow(sin_formula1,global.RESSWITCH_GLITCH_BAR_SMOOTH) * vSwitchres_show_bar ;
        } 
        else if (co.y < 0.0) {
            if (co.y > - vSwitchres_bar_size)
                switchresglitch = pow(sin_formula2, global.RESSWITCH_GLITCH_BAR_SMOOTH) * vSwitchres_show_bar ;
        }
        FragColor = texture(Source, co) + switchresglitch * vSwitchres_show_bar;
        
  
    //WOBBLYING

        float wobbly = 0.0;
        if (vWobblyAmp > 0.0) {
            wobbly = sin( co.y * 20.0 + vWobPhase) * vWobblyAmp;
            
            /* Different pattern
                #define a 4.1
                #define b 1.0
                #define c 1.0
                //wobbly = sin(0.03*k + a* sin(0.02*k + b * sin (0.01 *k + c* co.y))) * 0.1;
            */
            co.x+=wobbly;
        }
        
        FragColor = texture(Source, co) + switchresglitch * vSwitchres_show_bar;

}
