// See the main shader file for copyright and other information.

// clang-format off
#pragma parameter RT_CURV_SETTINGS "=== Ray Traced Curvature v1.0 settings ===" 0.0 0.0 1.0 1.0

#pragma parameter RT_CURV_CURVATURE "Curvature strength" 0.5 0.0 1.5 0.05
#pragma parameter RT_CURV_TILT_ANGLE_H "Horizontal tilt" 0.0 -0.5 0.5 0.01
#pragma parameter RT_CURV_TILT_ANGLE_V "Vertical tilt" -0.08 -0.5 0.5 0.01
#pragma parameter RT_CURV_ROUNDED_CORNER "Rounded corner radius" 0.02 0.0 0.2 0.01
#pragma parameter RT_CURV_SHAPE "Screen shape (0 = sphere, 1 = cylinder)" 0.0 0.0 1.0 1.0

#pragma parameter RT_CURV_ASPECT_H "Horizontal aspect ratio (0 = unchanged)" 0.0 0.0 256.0 1.0
#pragma parameter RT_CURV_ASPECT_V "Vertical aspect ratio (0 = unchanged)" 0.0 0.0 256.0 1.0

#pragma parameter RT_CURV_ZOOM "Zoom" 0.99 0.8 1.2 0.01
#define RT_CURV_F_MAX 10.0
#pragma parameter RT_CURV_F "Focal Length (max = ortho)" 2.0 0.5 10.0 0.25

#pragma parameter RT_CURV_LOD_BIAS "Anti-aliasing boost" 0.0 0.0 1.0 0.05
#pragma parameter RT_CURV_APPEND "Disable pixel art sampling (for append)" 0.0 0.0 1.0 1.0
// clang-format on
