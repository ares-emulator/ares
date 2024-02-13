#ifndef CROP_ZOOM_PAN
#define CROP_ZOOM_PAN

// wraps the standard TexCoord

#pragma parameter ia_overscan_percent_x "Stretch Width" 0.0 -25.0 25.0 1.0
#pragma parameter ia_overscan_percent_y "Stretch Height" 0.0 -25.0 25.0 1.0
#pragma parameter ia_ZOOM "Zoom In/Out" 1.0 0.0 4.0 0.01
#pragma parameter ia_XPOS "Pan X" 0.0 -2.0 2.0 0.005
#pragma parameter ia_YPOS "Pan Y" 0.0 -2.0 2.0 0.005

const vec2 shift = vec2(0.5);

vec2 crop_zoom_pan(vec2 in_coord){
   vec2 out_coord = in_coord - shift;
   out_coord /= ia_ZOOM;
   out_coord *= vec2(1.0 - ia_overscan_percent_x / 100.0, 1.0 - ia_overscan_percent_y / 100.0);
   out_coord += shift;
   out_coord += vec2(ia_XPOS, ia_YPOS);
   return out_coord;
}

#endif
