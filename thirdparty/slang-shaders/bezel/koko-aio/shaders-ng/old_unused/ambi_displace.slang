#version 450
#include "config.inc"
#define eps 1e-8

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vTexCoord_zoom;

#include "functions.include.slang"
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   vTexCoord_zoom = TexCoord;
   if (DO_BEZEL==1.0 && BEZEL_INNER_ZOOM !=0.0) vTexCoord_zoom = zoomout_coords(TexCoord, -BEZEL_INNER_ZOOM , 1.0);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vTexCoord_zoom;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Original;
layout(set = 0, binding = 3) uniform sampler2D main_pass;
layout(set = 0, binding = 4) uniform sampler2D isrotated_pass;



bool border_updown(){
   //Return if black bars are up and down or left and right.
   //Why this one work?
   return ((params.OriginalSize.x > params.OriginalSize.y) && (global.main_passSize.x < global.main_passSize.y )) || 
         (params.OriginalSize.x < params.OriginalSize.y) && (global.main_passSize.x > global.main_passSize.y ) ;
}

vec3 main_wrap(){

   float myborder = 0.025; //20% border to be displaced
   vec2 newcoords = vTexCoord_zoom;
   vec3 pixel_out;
   float mydiv;
   float myoffset;

   if (!border_updown()) {
      mydiv = vTexCoord.x / myborder;
      myoffset = (1.0-myborder) * floor(mydiv);

      if (vTexCoord.x < 0.5)
      newcoords.x = newcoords.x - (  myborder * floor(mydiv) ) ;
         else
      newcoords.x = (1 - myborder) +  ( newcoords.x -   (myborder*floor(mydiv))  )  ;
      
      return texture(Original, newcoords).rgb;
   }
   
   //Is rotated
      mydiv = vTexCoord.y / myborder;
      myoffset = (1.0-myborder) * floor(mydiv);

      if (vTexCoord.y < 0.5)
      newcoords.y = newcoords.y - (  myborder * floor(mydiv) ) ;
         else
      newcoords.y = (1 - myborder) +  ( newcoords.y -   (myborder*floor(mydiv))  )  ;   
      return texture(Original, newcoords).rgb;
   

}
#include "functions.include.slang"
	float mborder(vec2 coord,float aspect,float corner_size, float corner_smooth) {
		coord = (coord - vec2(0.5)) + vec2(0.5, 0.5);
		coord = min(coord, vec2(1.0) - coord) * aspect;
		vec2 cdist = vec2(corner_size);
		coord = (cdist - min(coord, cdist));
		float dist = sqrt(dot(coord, coord));
		return clamp((cdist.x - dist)*corner_smooth, 0.0, 1.0);
	}

	
	vec2 bget_scaled_coords(vec2 pTexCoord, vec4 destsize){
		if (!border_needed()) return pTexCoord;
			//else
		float scale_x = 1.0;
		float scale_y = 1.0;
		float offset_x = 0.0 ;
		float offset_y = 0.0 ;
		float in_aspect = get_in_aspect();
		if (is_rotated()) {
			//scale_y = params.OutputSize.y/(params.OutputSize.x / in_aspect );
			//scale_y = global.FinalViewportSize.y/(global.FinalViewportSize.x / in_aspect );
			scale_y = destsize.y/(destsize.x / in_aspect );
			offset_y = (0.5 * scale_y ) - 0.5 ;
		} else {
			//scale_x = params.OutputSize.x/(params.OutputSize.y * in_aspect);
			//scale_x = global.FinalViewportSize.x/(global.FinalViewportSize.y * in_aspect);
			scale_x = destsize.x/(destsize.y * in_aspect);
			offset_x = (0.5 * scale_x ) - 0.5 ;
		}

		vec2 scale_coord=vec2(pTexCoord.x*scale_x - offset_x , pTexCoord.y*scale_y - offset_y);
		return scale_coord;
	}
	

void main() {
   if (DO_AMBILIGHT != 1.0) return;
   //bool is_rotated = texture(isrotated_pass,vec2(0.5,0.5)).r > 0.5;
  

  //Disegna un rettangolo sul gioco.
  //samplo main_pass per riferimento.
  vec4 pixel_Original = texture(Original,vTexCoord);
  
  FragColor = pixel_Original;
  
/*
   float dist = 1-length(vTexCoord-0.5);

   FragColor = vec4(main_wrap(),1.0);
   
   float corner_size = 0.25;
   float corner_smooth = 4.0;
   float zoom = 0.4;
   vec2 co = get_scaled_coords(zoomout_coords(vTexCoord, -zoom , 1.0),global.FinalViewportSize);
   float ss=pow(mborder(co,1.0,corner_size, corner_smooth),2.0);
         
         //ss = ss * smoothstep(shade_end,shade_start, vTexCoord.x);
   
   FragColor = vec4(ss);
   
   
   vec2 ccords = get_scaled_coords(vTexCoord,global.FinalViewportSize);
   ccords = zoomout_coords(ccords,-BEZEL_INNER_ZOOM,1.0);
   //FragColor = texture(Original,ccords)*ss;
*/
}


