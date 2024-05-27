/*
    Mega Bezel - Creates a graphic treatment for the game play area to give a retro feel
    Copyright (C) 2019-2021 HyperspaceMadness - HyperspaceMadness@outlook.com

    Incorporates much great feedback from the libretro forum, and thanks
    to Hunterk who helped me get started

    See more at the libretro forum
    https://forums.libretro.com/t/hsm-mega-bezel-reflection-shader-feedback-and-updates

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma alias TextPass

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 2) uniform sampler2D TextPassFeedback;
#define PassFeedback TextPassFeedback

layout(set = 0, binding = 8) uniform sampler2D InfoCachePass;
layout(set = 0, binding = 9) uniform sampler2D InfoCachePassFeedback;

/*
    Text code is from the Shadertoy "96-bit 8x12" Font by Flyguy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define MAX_INT_DIGITS 4

#define CHAR_SIZE vec2(6, 7)
#define CHAR_SPACING vec2(6, 11)

#define STRWIDTH(c) (c * CHAR_SPACING.x)
#define STRHEIGHT(c) (c * CHAR_SPACING.y)

#define DOWN_SCALE 2.0

/*
Top left pixel is the most significant bit.
Bottom right pixel is the least significant bit.

 â–ˆâ–ˆâ–ˆ  |
â–ˆ   â–ˆ |
â–ˆ   â–ˆ |
â–ˆ   â–ˆ |
â–ˆâ–ˆâ–ˆâ–ˆâ–ˆ |
â–ˆ   â–ˆ |
â–ˆ   â–ˆ |

011100
100010
100010
100010
111110
100010
100010

011100 (upper 21 bits)
100010 -> 011100 100010 100010 100 -> 935188
100010
100
   010 (lower 21 bits)
111110 -> 010 111110 100010 100010 -> 780450
100010
100010

vec2(935188.0,780450.0)
*/

//Automatically generated from the sprite sheet here: http://uzebox.org/wiki/index.php?title=File:Font6x8.png
vec2 ch_spc = vec2(0.0,0.0);
vec2 ch_exc = vec2(276705.0,32776.0);
vec2 ch_quo = vec2(1797408.0,0.0);
vec2 ch_hsh = vec2(10738.0,1134484.0);
vec2 ch_dol = vec2(538883.0,19976.0);
vec2 ch_pct = vec2(1664033.0,68006.0);
vec2 ch_amp = vec2(545090.0,174362.0);
vec2 ch_apo = vec2(798848.0,0.0);
vec2 ch_lbr = vec2(270466.0,66568.0);
vec2 ch_rbr = vec2(528449.0,33296.0);
vec2 ch_ast = vec2(10471.0,1688832.0);
vec2 ch_crs = vec2(4167.0,1606144.0);
vec2 ch_per = vec2(0.0,1560.0);
vec2 ch_dsh = vec2(7.0,1572864.0);
vec2 ch_com = vec2(0.0,1544.0);
vec2 ch_lsl = vec2(1057.0,67584.0);
vec2 ch_0 = vec2(935221.0,731292.0);
vec2 ch_1 = vec2(274497.0,33308.0);
vec2 ch_2 = vec2(934929.0,1116222.0);
vec2 ch_3 = vec2(934931.0,1058972.0);
vec2 ch_4 = vec2(137380.0,1302788.0);
vec2 ch_5 = vec2(2048263.0,1058972.0);
vec2 ch_6 = vec2(401671.0,1190044.0);
vec2 ch_7 = vec2(2032673.0,66576.0);
vec2 ch_8 = vec2(935187.0,1190044.0);
vec2 ch_9 = vec2(935187.0,1581336.0);
vec2 ch_col = vec2(195.0,1560.0);
vec2 ch_scl = vec2(195.0,1544.0);
vec2 ch_les = vec2(135300.0,66052.0);
vec2 ch_equ = vec2(496.0,3968.0);
vec2 ch_grt = vec2(528416.0,541200.0);
vec2 ch_que = vec2(934929.0,1081352.0);
vec2 ch_ats = vec2(935285.0,714780.0);
vec2 ch_A = vec2(935188.0,780450.0);
vec2 ch_B = vec2(1983767.0,1190076.0);
vec2 ch_C = vec2(935172.0,133276.0);
vec2 ch_D = vec2(1983764.0,665788.0);
vec2 ch_E = vec2(2048263.0,1181758.0);
vec2 ch_F = vec2(2048263.0,1181728.0);
vec2 ch_G = vec2(935173.0,1714334.0);
vec2 ch_H = vec2(1131799.0,1714338.0);
vec2 ch_I = vec2(921665.0,33308.0);
vec2 ch_J = vec2(66576.0,665756.0);
vec2 ch_K = vec2(1132870.0,166178.0);
vec2 ch_L = vec2(1065220.0,133182.0);
vec2 ch_M = vec2(1142100.0,665762.0);
vec2 ch_N = vec2(1140052.0,1714338.0);
vec2 ch_O = vec2(935188.0,665756.0);
vec2 ch_P = vec2(1983767.0,1181728.0);
vec2 ch_Q = vec2(935188.0,698650.0);
vec2 ch_R = vec2(1983767.0,1198242.0);
vec2 ch_S = vec2(935171.0,1058972.0);
vec2 ch_T = vec2(2035777.0,33288.0);
vec2 ch_U = vec2(1131796.0,665756.0);
vec2 ch_V = vec2(1131796.0,664840.0);
vec2 ch_W = vec2(1131861.0,699028.0);
vec2 ch_X = vec2(1131681.0,84130.0);
vec2 ch_Y = vec2(1131794.0,1081864.0);
vec2 ch_Z = vec2(1968194.0,133180.0);
vec2 ch_lsb = vec2(925826.0,66588.0);
vec2 ch_rsl = vec2(16513.0,16512.0);
vec2 ch_rsb = vec2(919584.0,1065244.0);
vec2 ch_pow = vec2(272656.0,0.0);
vec2 ch_usc = vec2(0.0,62.0);
vec2 ch_a = vec2(224.0,649374.0);
vec2 ch_b = vec2(1065444.0,665788.0);
vec2 ch_c = vec2(228.0,657564.0);
vec2 ch_d = vec2(66804.0,665758.0);
vec2 ch_e = vec2(228.0,772124.0);
vec2 ch_f = vec2(401543.0,1115152.0);
vec2 ch_g = vec2(244.0,665474.0);
vec2 ch_h = vec2(1065444.0,665762.0);
vec2 ch_i = vec2(262209.0,33292.0);
vec2 ch_j = vec2(131168.0,1066252.0);
vec2 ch_k = vec2(1065253.0,199204.0);
vec2 ch_l = vec2(266305.0,33292.0);
vec2 ch_m = vec2(421.0,698530.0);
vec2 ch_n = vec2(452.0,1198372.0);
vec2 ch_o = vec2(228.0,665756.0);
vec2 ch_p = vec2(484.0,667424.0);
vec2 ch_q = vec2(244.0,665474.0);
vec2 ch_r = vec2(354.0,590904.0);
vec2 ch_s = vec2(228.0,114844.0);
vec2 ch_t = vec2(8674.0,66824.0);
vec2 ch_u = vec2(292.0,1198868.0);
vec2 ch_v = vec2(276.0,664840.0);
vec2 ch_w = vec2(276.0,700308.0);
vec2 ch_x = vec2(292.0,1149220.0);
vec2 ch_y = vec2(292.0,1163824.0);
vec2 ch_z = vec2(480.0,1148988.0);
vec2 ch_lpa = vec2(401542.0,66572.0);
vec2 ch_bar = vec2(266304.0,33288.0);
vec2 ch_rpa = vec2(788512.0,1589528.0);
vec2 ch_tid = vec2(675840.0,0.0);
vec2 ch_lar = vec2(8387.0,1147904.0);

float TEXT_MASK = 0;
vec2 CUR_UV = vec2(0);

#define _spc TEXT_MASK += char_px(ch_spc, CUR_UV);
#define _exc TEXT_MASK += char_px(ch_exc, CUR_UV);
#define _quo TEXT_MASK += char_px(ch_quo, CUR_UV);
#define _hsh TEXT_MASK += char_px(ch_hsh, CUR_UV);
#define _dol TEXT_MASK += char_px(ch_dol, CUR_UV);
#define _pct TEXT_MASK += char_px(ch_pct, CUR_UV);
#define _amp TEXT_MASK += char_px(ch_amp, CUR_UV);
#define _apo TEXT_MASK += char_px(ch_apo, CUR_UV);
#define _lbr TEXT_MASK += char_px(ch_lbr, CUR_UV);
#define _rbr TEXT_MASK += char_px(ch_rbr, CUR_UV);
#define _ast TEXT_MASK += char_px(ch_ast, CUR_UV);
#define _crs TEXT_MASK += char_px(ch_crs, CUR_UV);
#define _per TEXT_MASK += char_px(ch_per, CUR_UV);
#define _dsh TEXT_MASK += char_px(ch_dsh, CUR_UV);
#define _com TEXT_MASK += char_px(ch_com, CUR_UV);
#define _lsl TEXT_MASK += char_px(ch_lsl, CUR_UV);
#define _0 TEXT_MASK += char_px(ch_0, CUR_UV);
#define _1 TEXT_MASK += char_px(ch_1, CUR_UV);
#define _2 TEXT_MASK += char_px(ch_2, CUR_UV);
#define _3 TEXT_MASK += char_px(ch_3, CUR_UV);
#define _4 TEXT_MASK += char_px(ch_4, CUR_UV);
#define _5 TEXT_MASK += char_px(ch_5, CUR_UV);
#define _6 TEXT_MASK += char_px(ch_6, CUR_UV);
#define _7 TEXT_MASK += char_px(ch_7, CUR_UV);
#define _8 TEXT_MASK += char_px(ch_8, CUR_UV);
#define _9 TEXT_MASK += char_px(ch_9, CUR_UV);
#define _col TEXT_MASK += char_px(ch_col, CUR_UV);
#define _scl TEXT_MASK += char_px(ch_scl, CUR_UV);
#define _les TEXT_MASK += char_px(ch_les, CUR_UV);
#define _equ TEXT_MASK += char_px(ch_equ, CUR_UV);
#define _grt TEXT_MASK += char_px(ch_grt, CUR_UV);
#define _que TEXT_MASK += char_px(ch_que, CUR_UV);
#define _ats TEXT_MASK += char_px(ch_ats, CUR_UV);
#define _A TEXT_MASK += char_px(ch_A, CUR_UV);
#define _B TEXT_MASK += char_px(ch_B, CUR_UV);
#define _C TEXT_MASK += char_px(ch_C, CUR_UV);
#define _D TEXT_MASK += char_px(ch_D, CUR_UV);
#define _E TEXT_MASK += char_px(ch_E, CUR_UV);
#define _F TEXT_MASK += char_px(ch_F, CUR_UV);
#define _G TEXT_MASK += char_px(ch_G, CUR_UV);
#define _H TEXT_MASK += char_px(ch_H, CUR_UV);
#define _I TEXT_MASK += char_px(ch_I, CUR_UV);
#define _J TEXT_MASK += char_px(ch_J, CUR_UV);
#define _K TEXT_MASK += char_px(ch_K, CUR_UV);
#define _L TEXT_MASK += char_px(ch_L, CUR_UV);
#define _M TEXT_MASK += char_px(ch_M, CUR_UV);
#define _N TEXT_MASK += char_px(ch_N, CUR_UV);
#define _O TEXT_MASK += char_px(ch_O, CUR_UV);
#define _P TEXT_MASK += char_px(ch_P, CUR_UV);
#define _Q TEXT_MASK += char_px(ch_Q, CUR_UV);
#define _R TEXT_MASK += char_px(ch_R, CUR_UV);
#define _S TEXT_MASK += char_px(ch_S, CUR_UV);
#define _T TEXT_MASK += char_px(ch_T, CUR_UV);
#define _U TEXT_MASK += char_px(ch_U, CUR_UV);
#define _V TEXT_MASK += char_px(ch_V, CUR_UV);
#define _W TEXT_MASK += char_px(ch_W, CUR_UV);
#define _X TEXT_MASK += char_px(ch_X, CUR_UV);
#define _Y TEXT_MASK += char_px(ch_Y, CUR_UV);
#define _Z TEXT_MASK += char_px(ch_Z, CUR_UV);
#define _lsb TEXT_MASK += char_px(ch_lsb, CUR_UV);
#define _rsl TEXT_MASK += char_px(ch_rsl, CUR_UV);
#define _rsb TEXT_MASK += char_px(ch_rsb, CUR_UV);
#define _pow TEXT_MASK += char_px(ch_pow, CUR_UV);
#define _usc TEXT_MASK += char_px(ch_usc, CUR_UV);
#define _a TEXT_MASK += char_px(ch_a, CUR_UV);
#define _b TEXT_MASK += char_px(ch_b, CUR_UV);
#define _c TEXT_MASK += char_px(ch_c, CUR_UV);
#define _d TEXT_MASK += char_px(ch_d, CUR_UV);
#define _e TEXT_MASK += char_px(ch_e, CUR_UV);
#define _f TEXT_MASK += char_px(ch_f, CUR_UV);
#define _g TEXT_MASK += char_px(ch_g, CUR_UV);
#define _h TEXT_MASK += char_px(ch_h, CUR_UV);
#define _i TEXT_MASK += char_px(ch_i, CUR_UV);
#define _j TEXT_MASK += char_px(ch_j, CUR_UV);
#define _k TEXT_MASK += char_px(ch_k, CUR_UV);
#define _l TEXT_MASK += char_px(ch_l, CUR_UV);
#define _m TEXT_MASK += char_px(ch_m, CUR_UV);
#define _n TEXT_MASK += char_px(ch_n, CUR_UV);
#define _o TEXT_MASK += char_px(ch_o, CUR_UV);
#define _p TEXT_MASK += char_px(ch_p, CUR_UV);
#define _q TEXT_MASK += char_px(ch_q, CUR_UV);
#define _r TEXT_MASK += char_px(ch_r, CUR_UV);
#define _s TEXT_MASK += char_px(ch_s, CUR_UV);
#define _t TEXT_MASK += char_px(ch_t, CUR_UV);
#define _u TEXT_MASK += char_px(ch_u, CUR_UV);
#define _v TEXT_MASK += char_px(ch_v, CUR_UV);
#define _w TEXT_MASK += char_px(ch_w, CUR_UV);
#define _x TEXT_MASK += char_px(ch_x, CUR_UV);
#define _y TEXT_MASK += char_px(ch_y, CUR_UV);
#define _z TEXT_MASK += char_px(ch_z, CUR_UV);
#define _lpa TEXT_MASK += char_px(ch_lpa, CUR_UV);
#define _bar TEXT_MASK += char_px(ch_bar, CUR_UV);
#define _rpa TEXT_MASK += char_px(ch_rpa, CUR_UV);
#define _tid TEXT_MASK += char_px(ch_tid, CUR_UV);
#define _lar TEXT_MASK += char_px(ch_lar, CUR_UV);

vec2 res;
vec2 print_pos = vec2(0);

//Extracts bit b from the given number.
//Shifts bits right (num / 2^bit) then ANDs the result with 1 (mod(result,2.0)).
float extract_bit(float n, float b)
{
    b = clamp(b,-1.0,22.0); //Fixes small artefacts on my nexus 7
	return floor(mod(floor(n / pow(2.0,floor(b))),2.0));
}

//Returns the pixel at uv in the given bit-packed sprite.
float sprite(vec2 spr, vec2 size, vec2 uv)
{
    uv = floor(uv);
    //Calculate the bit to extract (x + y * width) (flipped on x-axis)
    float bit = (size.x-uv.x-1.0) + uv.y * size.x;

    //Clipping bound to remove garbage outside the sprite's boundaries.
    bool bounds = all(greaterThanEqual(uv,vec2(0)));
    bounds = bounds && all(lessThan(uv,size));

    return bounds ? extract_bit(spr.x, bit - 21.0)+extract_bit(spr.y, bit) : 0.0;

}

//Prints a character and moves the print position forward by 1 character width.
float char_px(vec2 ch, vec2 uv)
{
    float px = sprite(ch, CHAR_SIZE, uv - print_pos);
    print_pos.x += CHAR_SPACING.x;
    return px;
}

//Returns the digit sprite for the given number.
vec2 get_digit(float d)
{
    d = floor(d);

    if(d == 0.0) return ch_0;
    if(d == 1.0) return ch_1;
    if(d == 2.0) return ch_2;
    if(d == 3.0) return ch_3;
    if(d == 4.0) return ch_4;
    if(d == 5.0) return ch_5;
    if(d == 6.0) return ch_6;
    if(d == 7.0) return ch_7;
    if(d == 8.0) return ch_8;
    if(d == 9.0) return ch_9;
    return vec2(0.0);
}

//Prints out the given number starting at pos.
float print_number(float number,vec2 pos, vec2 uv)
{
	vec2 dec_pos = pos;
    float result = 0.0;

	for(int i = 3; i >= -2; i--)
    {
        float clip = float(abs(number) > pow(10.0, float(i)) || i == 0); //Clip off leading zeros.

        float digit = mod(number / pow(10.0, float(i)),10.0);

        if(i == -1)
        {
            result += sprite(ch_per,CHAR_SIZE, uv - dec_pos) * clip;
            dec_pos.x += CHAR_SPACING.x * clip;
        }
        result += sprite(get_digit(digit),CHAR_SIZE, uv - dec_pos) * clip;


        dec_pos.x += CHAR_SPACING.x * clip;
    }

    return result;
}

float print_integer(float number, int zeros, vec2 uv)
{
    float result = 0.0;

	for(int i = MAX_INT_DIGITS; i >= 0; i--)
    {
        float digit = mod( number / pow(10.0, float(i)) , 10.0);

        if(abs(number) > pow(10.0, float(i)) || zeros > i  || i == 0) //Clip off leading zeros.
        {
        	result += char_px(get_digit(digit), uv);
        }
    }
    return result;
}

#define eps 1e-3

float DrawResolutions(vec2 uv, vec2 viewport_size, vec2 screen_size, vec4 screen_region, vec2 derezed_size, vec2 negative_crop_added_size, vec2 cropped_original_size, vec2 sampling_res, vec2 int_mult)
{
    float col = 0.0;

    float current_line_index = 0;
    float margin = STRHEIGHT(1);

    // Resolution Text
    print_pos = vec2(margin, 0);

    // Viewport Resolution ----------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   TEXT_MASK = 0;
   CUR_UV = uv;
   _V _i _e _w _p _o _r _t _spc _dsh _dsh _dsh _dsh _dsh _dsh _spc
   TEXT_MASK += print_integer(viewport_size.x, 0, uv);
   _spc _x _spc
   TEXT_MASK += print_integer(viewport_size.y, 0, uv);

    // Screen Region ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _S _c _r _e _e _n _spc _R _e _g _i _o _n _spc _dsh _spc
   TEXT_MASK += print_integer(screen_region.x, 0, uv);
   _spc
   TEXT_MASK += print_integer(screen_region.y, 0, uv);
   _spc
   TEXT_MASK += print_integer(screen_region.z, 0, uv);
   _spc
   TEXT_MASK += print_integer(screen_region.w, 0, uv);

    // Screen Resolution ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _S _c _r _e _e _n _spc _dsh _dsh _dsh _dsh _dsh _dsh _dsh _dsh _spc
   TEXT_MASK += print_integer(screen_size.x, 0, uv);
   _spc _x _spc
   TEXT_MASK += print_integer(screen_size.y, 0, uv);

    // Final Aspect Ratio ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _F _i _n _a _l _spc _A _s _p _e _c _t _spc _dsh _dsh _spc
   TEXT_MASK += print_number(screen_size.x/screen_size.y > 1 ? screen_size.x/screen_size.y : screen_size.y/screen_size.x, print_pos, uv);

    // Non-Int Scale Percent ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

    _N _o _n _spc _I _n _t _spc _S _c _l _spc _pct _dsh _dsh _spc
    TEXT_MASK += print_number(screen_size.y / viewport_size.y * 100, print_pos, uv);

    // Int Scale Vertical ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _I _n _t _spc _S _c _l _spc _V _e _r _t _spc _dsh _dsh _spc

    if (fract(int_mult.y) < 0.01)
        TEXT_MASK += print_integer(int_mult.y, 0, uv);
    else
        TEXT_MASK += print_number(int_mult.y, print_pos, uv);

    // Int Scale Horizontal ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _I _n _t _spc _S _c _l _spc _H _o _r _z _spc _dsh _dsh _spc

    if (fract(int_mult.x) < 0.01)
        TEXT_MASK += print_integer(int_mult.x, 0, uv);
    else
        TEXT_MASK += print_number(int_mult.x, print_pos, uv);

    // Sampling Resolution ----------------------------------------------------------
    if (cropped_original_size.x != sampling_res.x || cropped_original_size.y != sampling_res.y)
    {
      print_pos.x = margin;
      print_pos.y += STRHEIGHT(1);

      _S _a _m _p _l _i _n _g _spc _R _e _s _spc _dsh _dsh _spc

      TEXT_MASK += print_integer(sampling_res.x, 0, uv);
      _spc _x _spc
      TEXT_MASK += print_integer(sampling_res.y, 0, uv);
    }


    // CROPPED Resolution ----------------------------------------------------------
    if (cropped_original_size.x != ROTATED_CORE_PREPPED_SIZE.x || cropped_original_size.y != ROTATED_CORE_PREPPED_SIZE.y)
    {
        print_pos.x = margin;
        print_pos.y += STRHEIGHT(1);

      _C _r _o _p _p _e _d _spc _dsh _dsh _dsh _dsh _dsh _dsh _dsh _spc

      TEXT_MASK += print_integer(cropped_original_size.x, 0, uv);
      _spc _x _spc
      TEXT_MASK += print_integer(cropped_original_size.y, 0, uv);
    }


    // Negative Crop Resolution ----------------------------------------------------------
    if (derezed_size.x != ROTATED_CORE_PREPPED_SIZE.x || derezed_size.y != ROTATED_CORE_PREPPED_SIZE.y)
    {
      print_pos.x = margin;
      print_pos.y += STRHEIGHT(1);

      _N _e _g _dsh _C _r _o _p _spc _A _d _d _spc _dsh _dsh _spc

      TEXT_MASK += print_integer(ROTATED_CORE_PREPPED_SIZE.x, 0, uv);
      _spc _x _spc
      TEXT_MASK += print_integer(ROTATED_CORE_PREPPED_SIZE.y, 0, uv);
    }


    // DEREZ Resolution ----------------------------------------------------------
    if (derezed_size.x != ROTATED_CORE_ORIGINAL_SIZE.x || derezed_size.y != ROTATED_CORE_ORIGINAL_SIZE.y)
    {
        print_pos.x = margin;
        print_pos.y += STRHEIGHT(1);

      _D _e _r _e _z _e _d _spc _dsh _dsh _dsh _dsh _dsh _dsh _dsh _spc

      TEXT_MASK += print_integer(derezed_size.x, 0, uv);
      _spc _x _spc
      TEXT_MASK +=  print_integer(derezed_size.y, 0, uv);
    }


    // CORE Resolution ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _C _o _r _e _spc _dsh _dsh _dsh _dsh _dsh _dsh _dsh _dsh _dsh _dsh _spc

   TEXT_MASK += print_integer(ROTATED_CORE_ORIGINAL_SIZE.x, 0, uv);
   _spc _x _spc
   TEXT_MASK += print_integer(ROTATED_CORE_ORIGINAL_SIZE.y, 0, uv);

    // Line 03 Preset Type ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(2);

#ifdef IS_ADV_PRESET
   _A _D _V _A _N _C _E _D
#endif

#ifdef IS_STD_PRESET
   _S _T _A _N _D _A _R _D
#endif

#ifdef IS_GLASS_PRESET
   _spc _G _L _A _S _S
#endif

#ifdef IS_NO_REFLECT_PRESET
   _spc _N _O _spc _R _E _F _L _E _C _T
#endif

#ifdef IS_POTATO_PRESET
   _P _O _T _A _T _O
#endif

    // Version ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   // Text Version V1.17.2_2024-05-18
   // Version
   _1 _per _1 _7 _per _2 _spc
   // Date
   _2 _0 _2 _4 _dsh _0 _5 _dsh _1 _8

    // Title ----------------------------------------------------------
    print_pos.x = margin;
    print_pos.y += STRHEIGHT(1);

   _M _E _G _A _spc _B _E _Z _E _L _spc

    return TEXT_MASK;
}

vec2 GetIntegerScaleMult(vec2 screen_size, vec2 cropped_size)
{
	float epsilon = 0.01;
	vec2 integer_scale = screen_size / cropped_size;
	integer_scale = vec2(abs(integer_scale.x - round(integer_scale.x)) < epsilon ? round(integer_scale.x) : integer_scale.x,
						 abs(integer_scale.y - round(integer_scale.y)) < epsilon ? round(integer_scale.y) : integer_scale.y);
	return integer_scale;
}

vec4 GetScreenRegion(vec2 screen_size)
{
    vec2 position_offset = SCREEN_POS_OFFSET * global.FinalViewportSize.xy * HSM_VIEWPORT_ZOOM +
                           vec2(-HSM_VIEWPORT_POSITION_X, HSM_VIEWPORT_POSITION_Y) * HSM_VIEWPORT_ZOOM * global.FinalViewportSize.xy;

    vec2 top_left = global.FinalViewportSize.xy / 2 - screen_size / 2 + position_offset;

    vec2 bottom_right = global.FinalViewportSize.xy / 2 + screen_size / 2 + position_offset;

    vec4 out_region = round(vec4(top_left, bottom_right));
    return out_region;
}

float GetText(vec2 in_coord)
{
   in_coord.x = HSM_FLIP_VIEWPORT_HORIZONTAL * HSM_FLIP_CORE_HORIZONTAL * (in_coord.x - 0.5) + 0.5;
   in_coord.y = HSM_FLIP_VIEWPORT_VERTICAL * HSM_FLIP_CORE_VERTICAL * (in_coord.y - 0.5) + 0.5;

    vec2 int_mult = GetIntegerScaleMult(SCREEN_SIZE, CROPPED_ROTATED_SIZE);
    vec2 canvas_size = global.DerezedPassSize.xy;

    in_coord = (in_coord - 0.5) * vec2(1, -1) + 0.5;

    vec2 downscale = vec2(1, 1);
    downscale.x = max(max(floor(canvas_size.x / 240), 1), 1);
    downscale.y = max(max(ceil(canvas_size.y / 200), 1), 1);

    vec2 uv = global.OutputSize.xy * in_coord;
	vec2 downscaled_uv = floor(global.OutputSize.xy * in_coord / downscale);

    vec4 screen_region = GetScreenRegion(SCREEN_SIZE);

	float pixel = DrawResolutions(downscaled_uv, global.FinalViewportSize.xy, SCREEN_SIZE, screen_region, ROTATED_DEREZED_SIZE, ROTATED_CORE_PREPPED_SIZE, CROPPED_ROTATED_SIZE, CROPPED_ROTATED_SIZE_WITH_RES_MULT, int_mult);

    vec3 col = vec3(1);
    pixel *= (1 - distance(mod(uv, vec2(1.0)), vec2(0.65))) * 1.2;
    col *= mix(vec3(0), vec3(1, 1, 1), pixel);

	return col.r;
}

void main()
{
	vec2 UNFLIPPED_VIEWPORT_COORD = vTexCoord;
    vec2 VIEWPORT_COORD = vTexCoord;
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

   if (!CACHE_INFO_CHANGED)
   {
      FragColor = texture(PassFeedback, UNFLIPPED_VIEWPORT_COORD);
      return;
   }

   // Flip the coordinate vertically if desired
   VIEWPORT_COORD.y = HSM_FLIP_VIEWPORT_VERTICAL * HSM_FLIP_CORE_VERTICAL * (VIEWPORT_COORD.y - 0.5) + 0.5;
   VIEWPORT_COORD.x = HSM_FLIP_VIEWPORT_HORIZONTAL * HSM_FLIP_CORE_HORIZONTAL * (VIEWPORT_COORD.x - 0.5) + 0.5;

   //////// Draw text to show resolutions //////////
   FragColor = vec4(GetText(VIEWPORT_COORD - vec2(MAX_NEGATIVE_CROP, -MAX_NEGATIVE_CROP)));
}