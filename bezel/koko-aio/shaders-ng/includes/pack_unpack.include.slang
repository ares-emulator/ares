/* pack_2to1_f and unpack_1to2_float 
 * Take 2 floats and pack them into one
 * #pragma format R32G32B32A32_SFLOAT needed
 * The brighter color is lost.
*/
	#define pack_floats_clamp 0.996078431372549 //1-(1/255)
	#define pack_floats_precision 255.0
	float pack_2to1_f(vec2 in_float){
		//Clamp and round to 1/255 needed:
		in_float = min(in_float,pack_floats_clamp);
		in_float = floor(in_float*255.0)/255.0;
		return in_float.x*pack_floats_precision + in_float.y;
	}

	vec2 unpack_1to2_float(float f){
		float fr = fract(f);
		float fg = floor(f)/pack_floats_precision;
		return vec2(fr,fg);
	}
	
	//pack 2 vec3 into one
	vec3 pack_2to1_vec3(vec3 v1, vec3 v2) {
		return vec3( 
			pack_2to1_f(vec2(v1.r,v2.r)),
			pack_2to1_f(vec2(v1.g,v2.g)),
			pack_2to1_f(vec2(v1.b,v2.b))
		       );
	}
	void unpack_1to2_vec3(in vec3 v, out vec3 v1, out vec3 v2) {
		vec2 t;
		t.xy = unpack_1to2_float(v.r);
		       v1.r = t.x;
		       v2.r = t.y;
		t.xy = unpack_1to2_float(v.g);
		       v1.g = t.x;
		       v2.g = t.y;
		t.xy = unpack_1to2_float(v.b);
		       v1.b = t.x;
		       v2.b= t.y;
	}

/* packColor and unpackColor 
 * Take a vec3 and pack them into one float
 * #pragma format R32G32B32A32_SFLOAT needed
 * The brighter colors (2) are lost.
*/
	#define mul_low 65025.0      //(pow(255.0,2.0))
	#define mul_mid 16581375.0   //(pow(255.0,3.0))
	#define mul_hi  4228250625.0 //(pow(255.0,4.0))

	#define pack_color_clamp 0.992156862745098 // 1.0-(2.0/255)

	float packColor(vec3 color) {
	color = clamp(color,0.0, pack_color_clamp);
	color = floor(color*255.0)/255.0;
	return (color.b * mul_low) + (color.g * mul_mid) + (color.r * mul_hi) ;
	}

	vec3 unpackColor(float result) {
		float hi =  floor(result/mul_mid)*mul_mid;
		float mid = floor((result - hi)/mul_low)*mul_low;
		float low = result - hi - mid;
		return vec3(hi/mul_hi, mid/mul_mid, low/mul_low);
	}
