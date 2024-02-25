
#define kMaxNitsFor2084     10000.0f

const mat3 k709_to_2020 = mat3 (
   0.6274040f, 0.3292820f, 0.0433136f,
   0.0690970f, 0.9195400f, 0.0113612f,
   0.0163916f, 0.0880132f, 0.8955950f);

/* START Converted from (Copyright (c) Microsoft Corporation - Licensed under the MIT License.)  https://github.com/microsoft/Xbox-ATG-Samples/tree/master/Kits/ATGTK/HDR */
const mat3 kExpanded709_to_2020 = mat3 (
    0.6274040f,  0.3292820f, 0.0433136f,
    0.0457456f,  0.941777f,  0.0124772f,
   -0.00121055f, 0.0176041f, 0.983607f);

const mat3 k2020Gamuts[2] = { k709_to_2020, kExpanded709_to_2020 };

float LinearToST2084_1(const float channel)
{
   float ST2084 = pow((0.8359375f + 18.8515625f * pow(abs(channel), 0.1593017578f)) / (1.0f + 18.6875f * pow(abs(channel), 0.1593017578f)), 78.84375f);
   return ST2084;  /* Don't clamp between [0..1], so we can still perform operations on scene values higher than 10,000 nits */
}

vec3 LinearToST2084(const vec3 colour)
{
	return vec3(LinearToST2084_1(colour.r), LinearToST2084_1(colour.g), LinearToST2084_1(colour.b));
}

/* END Converted from (Copyright (c) Microsoft Corporation - Licensed under the MIT License.)  https://github.com/microsoft/Xbox-ATG-Samples/tree/master/Kits/ATGTK/HDR */

/* Convert into HDR10 */
vec3 Hdr10(const vec3 hdr_linear, float paper_white_nits, float expand_gamut)
{
   const vec3 rec2020       = hdr_linear * k2020Gamuts[uint(expand_gamut)];
   const vec3 linearColour  = rec2020 * (paper_white_nits / kMaxNitsFor2084);
   vec3 hdr10               = LinearToST2084(linearColour);

   return hdr10;
}

