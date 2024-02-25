#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float simthresh, linewidth, ce_rotation, ce_zoom, ce_x_off, ce_y_off, ce_mode;
} params;

#pragma parameter linewidth "Line Width" 1.0 0.01 2.0 0.1
float lineWidth = params.linewidth;

// +/-3.14 = 180 degrees; +/-6.28 = 360 degrees
#pragma parameter ce_rotation "Rotation" 0.0 -6.28 6.28 0.01
float ROTATION = params.ce_rotation;

#pragma parameter ce_zoom "Zoom" 1.0 0.0 2.0 0.01
float ZOOM = params.ce_zoom;
#pragma parameter ce_x_off "X Offset" 0.0 -1.0 1.0 0.002
float X_OFF = params.ce_x_off;
#pragma parameter ce_y_off "Y Offset" 0.0 -1.0 1.0 0.002
float Y_OFF = params.ce_y_off;

//how close two colors should be to be considered "similar".
// can group shapes of visually similar colors, but creates
// some artifacting and should be kept as low as possible.
#pragma parameter simthresh "Similarity Threshold" 0.0 0.0 1.0 0.01
float similarThreshold = params.simthresh;

#pragma parameter ce_mode "cleanEdge Effect Toggle (debug)" 1.0 0.0 1.0 1.0
bool mode = bool(params.ce_mode);

#include "../../include/cleanEdge.inc"
