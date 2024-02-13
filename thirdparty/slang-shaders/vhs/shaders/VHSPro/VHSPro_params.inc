layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
   float screenLinesNum, VHS_Bleed_toggle, VHS_BleedMode_select, bleedAmount, VHS_FishEye_toggle,
      VHS_FishEye_Hyperspace_toggle, fisheyeBend, cutoffX, cutoffY, cutoffFadeX, cutoffFadeY,
      VHS_Vignette_toggle, vignetteAmount, vignetteSpeed, noiseLinesLum, noiseQuantizeX,
      VHS_FilmGrain_toggle, filmGrainAmount, VHS_YIQNoise_toggle, signalNoiseType_select,
      signalNoiseAmount, signalNoisePower, VHS_LineNoise_toggle, lineNoiseAmount, lineNoiseSpeed,
      VHS_TapeNoise_toggle, tapeNoiseTH, tapeNoiseAmount, tapeNoiseSpeed, VHS_ScanLines_toggle,
      scanLineWidth, VHS_LinesFloat_toggle, linesFloatSpeed, VHS_Stretch_toggle,VHS_Jitter_H_toggle,
      jitterHAmount, VHSJitter_V_toggle, jitterVAmount, jitterVSpeed, VHS_Twitch_H_toggle,
      twitchHFreq, VHS_Twitch_V_toggle, twitchVFreq, VHS_SignalTweak_toggle, signalAdjustY,
      signalAdjustI, signalAdjustQ, signalShiftY, signalShiftI, signalShiftQ, gammaCorection,
      VHS_Feedback_toggle, feedbackAmount, feedbackFade, feedbackThresh, feedbackColor_R,
      feedbackColor_G, feedbackColor_B, feedbackDebug_toggle, noiseLinesNum;
} global;

#pragma parameter screenLinesNum "Screen Resolution [VHSPro]" 1080.0 1.0 2160.0 16.0
#define screenLinesNum global.screenLinesNum
// "Screen Resolution (in lines).\nChange screenLinesRes in Preprocessor Definitions to have the same value as this."

#pragma parameter VHS_Bleed_toggle "Toggle Bleeding [VHSPro]" 1.0 0.0 1.0 1.0
bool VHS_Bleed = bool(global.VHS_Bleed_toggle);
// "Enables beam screen bleeding (makes the image blurry)."

#pragma parameter VHS_BleedMode_select "Bleeding Mode [VHSPro]" 1.0 1.0 5.0 1.0
int VHS_BleedMode = int(global.VHS_BleedMode_select);
// "Toggles between different bleeding modes."

#pragma parameter bleedAmount "Bleed Stretch [VHSPro]" 0.5 0.0 15.0 0.1
#define bleedAmount global.bleedAmount
// "Length of the bleeding." // default value of 1.0 was too much

#pragma parameter VHS_FishEye_toggle "Toggle Fisheye [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_FishEye = bool(global.VHS_FishEye_toggle);
// "Enables a CRT Curvature." // defaulting to off

#pragma parameter VHS_FishEye_Hyperspace_toggle "Toggle Fisheye Hyperspace [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_FishEye_Hyperspace = bool(global.VHS_FishEye_Hyperspace_toggle);
// "Changes the curvature to look like some sort of hyperspace warping."

#pragma parameter fisheyeBend "Fisheye Bend [VHSPro]" 0.5 0.0 3.0 0.05
#define fisheyeBend global.fisheyeBend
// "Curvature of the CRT." // default value of 2.0 was way too high

#pragma parameter cutoffX "Fisheye Cutoff X [VHSPro]" 2.0 0.0 50.0 1.0
#define cutoffX global.cutoffX
// "Cutoff of the Horizontal Borders."

#pragma parameter cutoffY "Fisheye Cutoff Y [VHSPro]" 3.0 0.0 50.0 1.0
#define cutoffY global.cutoffY
// "Cutoff of the Vertical Borders."

#pragma parameter cutoffFadeX "Fisheye Cutoff Fade X [VHSPro]" 25.0 0.0 50.0 1.0
#define cutoffFadeX global.cutoffFadeX
// "Size of the Horizontal gradient cutoff."

#pragma parameter cutoffFadeY "Fisheye Cutoff Fade Y [VHSPro]" 25.0 0.0 50.0 1.0
#define cutoffFadeY global.cutoffFadeY
// "Size of the Vertical gradient cutoff."

#pragma parameter VHS_Vignette_toggle "Toggle Vignette [VHSPro]" 1.0 0.0 1.0 1.0
bool VHS_Vignette = bool(global.VHS_Vignette_toggle);
// "Enables screen vignetting" // default to 'on' to make up for lost fisheye

#pragma parameter vignetteAmount "Vignette Amount [VHSPro]" 0.3 0.0 5.0 0.05
#define vignetteAmount global.vignetteAmount
// "Strength of the vignette." // default value of 1.0 was way too much

#pragma parameter vignetteSpeed "Vignette Pulse Speed [VHSPro]" 1.0 0.0 5.0 0.05
#define vignetteSpeed global.vignetteSpeed
// "Speed of the vignette pulsing. (Setting it to 0 makes it stop pulsing)"

#pragma parameter noiseLinesNum "Vertical Resolution [VHSPro]" 240.0 1.0 1080.0 16.0
#define noiseLinesNum global.noiseLinesNum
// "Noise Resolution (in lines).\nChange noiseLinesRes in Preprocessor Definitions to have the same value as this."

#pragma parameter noiseQuantizeX "Quantize Noise X [VHSPro]" 0.0 0.0 1.0 0.01
#define noiseQuantizeX global.noiseQuantizeX
// "Makes the noise longer or shorter."

#pragma parameter VHS_FilmGrain_toggle "Toggle Film Grain [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_FilmGrain = bool(global.VHS_FilmGrain_toggle);
// "Enables a Film Grain on the screen."

#pragma parameter filmGrainAmount "Film Grain Alpha [VHSPro]" 0.5 0.0 1.0 0.01
float filmGrainAmount = global.filmGrainAmount * 0.1;
// "Intensity of the Film Grain." // default was too low

#pragma parameter VHS_YIQNoise_toggle "Toggle Signal Noise [VHSPro]" 1.0 0.0 1.0 1.0
bool VHS_YIQNoise = bool(global.VHS_YIQNoise_toggle);
// "Adds noise to the YIQ Signal, causing a Pink (or green) noise."

#pragma parameter signalNoiseType_select "Signal Noise Type [VHSPro]" 1.0 1.0 3.0 1.0
int signalNoiseType = int(global.signalNoiseType_select);
// "Signal Noise Type [VHS Pro]"

#pragma parameter signalNoiseAmount "Signal Noise Amount [VHSPro]" 0.10 0.0 1.0 0.01
#define signalNoiseAmount global.signalNoiseAmount
// "Amount of the signal noise."

#pragma parameter signalNoisePower "Signal Noise Power [VHSPro]" 0.75 0.0 1.0 0.01
#define signalNoisePower global.signalNoisePower
// "Power of the signal noise. Higher values will make it green, lower values will make it more pink."

#pragma parameter VHS_LineNoise_toggle "Toggle Line Noise [VHSPro]" 1.0 0.0 1.0 1.0
bool VHS_LineNoise = bool(global.VHS_LineNoise_toggle);
// "Enables blinking line noise in the image."

#pragma parameter lineNoiseAmount "Line Noise Amount [VHSPro]" 1.0 0.0 10.0 0.1
#define lineNoiseAmount global.lineNoiseAmount
// "Intensity of the line noise."

#pragma parameter lineNoiseSpeed  "Line Noise Speed [VHSPro]" 5.0 0.0 10.0 0.1
#define lineNoiseSpeed global.lineNoiseSpeed
// "Speed of the line noise blinking delay."

#pragma parameter VHS_TapeNoise_toggle "Toggle Tape Noise [VHSPro]" 1.0 0.0 1.0 1.0
bool VHS_TapeNoise = bool(global.VHS_TapeNoise_toggle);
// "Adds scrolling noise like in old VHS Tapes."

#pragma parameter tapeNoiseTH "Tape Noise Amount [VHSPro]" 0.63 0.0 1.5 0.01
#define tapeNoiseTH global.tapeNoiseTH
// "Intensity of Tape Noise in the image."

#pragma parameter tapeNoiseAmount "Tape Noise Alpha [VHSPro]" 0.05 0.0 1.5 0.01
#define tapeNoiseAmount global.tapeNoiseAmount
// "Amount of Tape Noise in the image."

#pragma parameter tapeNoiseSpeed "Tape Noise Speed [VHSPro]" 1.0 -1.5 1.5 0.05
#define tapeNoiseSpeed global.tapeNoiseSpeed
// "Scrolling speed of the Tape Noise."

#pragma parameter VHS_ScanLines_toggle "Toggle Scanlines [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_ScanLines = bool(global.VHS_ScanLines_toggle);
// "Enables TV/CRT Scanlines."

#pragma parameter scanLineWidth "Scanlines Width [VHSPro]" 10.0 0.0 20.0 0.25
#define scanLineWidth global.scanLineWidth
// "Width of the Scanlines."

#pragma parameter VHS_LinesFloat_toggle "Toggle Lines Float [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_LinesFloat = bool(global.VHS_LinesFloat_toggle);
// "Makes the lines of the screen floats up or down. Works best with low Screen Lines resolutions."

#pragma parameter linesFloatSpeed "Lines Float Speed [VHSPro]" 1.0 -3.0 3.0 0.1
#define linesFloatSpeed global.linesFloatSpeed
// "Speed (and direction) of the floating lines."

#pragma parameter VHS_Stretch_toggle "Toggle Stretch Noise" 1.0 0.0 1.0 1.0
bool VHS_Stretch = bool(global.VHS_Stretch_toggle);
// "Enables a stretching noise that scrolls up and down on the Image, simulating magnetic interference of VHS tapes."

#pragma parameter VHS_Jitter_H_toggle "Toggle Interlacing [VHSPro]" 1.0 0.0 1.0 1.0
bool VHS_Jitter_H = bool(global.VHS_Jitter_H_toggle);
// "Enables interlacing."

#pragma parameter jitterHAmount "Interlacing Amount [VHSPro]" 0.15 0.0 5.0 0.05
#define jitterHAmount global.jitterHAmount
// "Strength of the Interlacing." //default 0.50 was too much

#pragma parameter VHSJitter_V_toggle "Toggle Jitter [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_Jitter_V = bool(global.VHSJitter_V_toggle);
// "Adds vertical jittering noise."

#pragma parameter jitterVAmount "Jitter Amount [VHSPro]" 7.5 0.0 15.0 0.1
#define jitterVAmount global.jitterVAmount
// "Amount of the vertical jittering noise." //default 1.0 was invisible

#pragma parameter jitterVSpeed "Jitter Speed [VHSPro]" 1.0 0.0 5.0 0.1
#define jitterVSpeed global.jitterVSpeed
// "Speed of the vertical jittering noise."

#pragma parameter VHS_Twitch_H_toggle "Toggle Horizontal Twitch [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_Twitch_H = bool(global.VHS_Twitch_H_toggle);
// "Makes the image twitches horizontally in certain timed intervals."

#pragma parameter twitchHFreq "Horizontal Twitch Frequency [VHSPro]" 1.0 0.0 5.0 0.1
#define twitchHFreq global.twitchHFreq
// "Frequency of time in which the image twitches horizontally."

#pragma parameter VHS_Twitch_V_toggle "Toggle Vertical Twitch [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_Twitch_V = bool(global.VHS_Twitch_V_toggle);
// "Makes the image twitches vertically in certain timed intervals."

#pragma parameter twitchVFreq "Vertical Twitch Frequency [VHSPro]" 1.0 0.0 5.0 0.1
#define twitchVFreq global.twitchVFreq
// "Frequency of time in which the image twitches vertically."

#pragma parameter VHS_SignalTweak_toggle "Toggle Signal Tweak [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_SignalTweak = bool(global.VHS_SignalTweak_toggle);
// "Tweak the values of the YIQ signal."

#pragma parameter signalAdjustY "Signal Shift Y [VHSPro]" 0.0 -0.25 0.25 0.01
#define signalAdjustY global.signalAdjustY
// "Shifts/Tweaks the Luma part of the signal."

#pragma parameter signalAdjustI "Signal Shift I [VHSPro]" 0.0 -0.25 0.25 0.01
#define signalAdjustI global.signalAdjustI
// "Shifts/Tweaks the Chroma part of the signal."

#pragma parameter signalAdjustQ "Signal Shift Q [VHSPro]" 0.0 -0.25 0.25 0.01
#define signalAdjustQ global.signalAdjustQ
// "Shifts/Tweaks the Chroma part of the signal."

#pragma parameter signalShiftY "Signal Adjust Y [VHSPro]" 1.0 -2.0 2.0 0.05
#define signalShiftY global.signalShiftY
// "Adjusts the Luma part of the signal."

#pragma parameter signalShiftI "Signal Adjust I [VHSPro]" 1.0 -2.0 2.0 0.05
#define signalShiftI global.signalShiftI
// "Adjusts the Chroma part of the signal."

#pragma parameter signalShiftQ "Signal Adjust Q [VHSPro]" 1.0 -2.0 2.0 0.05
#define signalShiftQ global.signalShiftQ
// "Adjusts the Chroma part of the signal."

#pragma parameter gammaCorection "Signal Gamma Correction [VHSPro]" 1.0 0.0 2.0 0.05
#define gammaCorection global.gammaCorection
// "Gamma corrects the image."

#pragma parameter VHS_Feedback_toggle "Toggle Phosphor Trails [VHSPro]" 0.0 0.0 1.0 1.0
bool VHS_Feedback = bool(global.VHS_Feedback_toggle);
// "Enables phosphor-trails from old CRT monitors."

#pragma parameter feedbackAmount "Input Amount [VHSPro]" 2.0 0.0 3.0 0.05
#define feedbackAmount global.feedbackAmount
// "Amount of Phosphor Trails."

#pragma parameter feedbackFade "Phosphor Fade [VHSPro]" 0.82 0.0 1.0 0.01
#define feedbackFade global.feedbackFade
// "Fade-time of the phosphor-trails."

#pragma parameter feedbackThresh "Input Cutoff [VHSPro]" 0.1 0.0 1.0 0.01
#define feedbackThresh global.feedbackThresh
// "Cutoff of the trail."

#pragma parameter feedbackColor_R "Red Tint of Phosphor Trail" 1.0 0.0 1.0 0.01
#pragma parameter feedbackColor_G "Green Tint of Phosphor Trail" 0.5 0.0 1.0 0.01
#pragma parameter feedbackColor_B "Blue Tint of Phosphor Trail" 0.0 0.0 1.0 0.01
vec3 feedbackColor = vec3(global.feedbackColor_R, global.feedbackColor_G, global.feedbackColor_B);
// "Color of the trail."

#pragma parameter feedbackDebug_toggle "Toggle Debug Trail [VHSPro]" 0.0 0.0 1.0 1.0
bool feedbackDebug = bool(global.feedbackDebug_toggle);
// "Enables the visualization of the phosphor-trails only."