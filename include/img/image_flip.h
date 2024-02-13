#ifndef IMAGE_FLIP
#define IMAGE_FLIP

// wraps the Position in gl_Position calculation

#pragma parameter ia_FLIP_HORZ "Flip Horiz Axis" 0.0 0.0 1.0 1.0
#pragma parameter ia_FLIP_VERT "Flip Vert Axis" 0.0 0.0 1.0 1.0
   
vec4 flip_pos(vec4 in_pos){
   vec4 out_pos = in_pos;
   out_pos.x = (ia_FLIP_HORZ < 0.5) ? out_pos.x : 1.0 - out_pos.x;
   out_pos.y = (ia_FLIP_VERT < 0.5) ? out_pos.y : 1.0 - out_pos.y;
   return out_pos;
}

#endif
