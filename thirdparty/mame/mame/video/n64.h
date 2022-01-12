// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
#ifndef _VIDEO_N64_H_
#define _VIDEO_N64_H_

#include "video/poly.h"
#include "pin64.h"

/*****************************************************************************/

#define PIXEL_SIZE_4BIT         0
#define PIXEL_SIZE_8BIT         1
#define PIXEL_SIZE_16BIT        2
#define PIXEL_SIZE_32BIT        3

#define CYCLE_TYPE_1            0
#define CYCLE_TYPE_2            1
#define CYCLE_TYPE_COPY         2
#define CYCLE_TYPE_FILL         3

#define SAMPLE_TYPE_1x1         0
#define SAMPLE_TYPE_2x2         1

#define BYTE_ADDR_XOR       BYTE4_XOR_BE(0)
#define WORD_ADDR_XOR       (WORD_XOR_BE(0) >> 1)

#define XOR_SWAP_BYTE_SHIFT     2
#define XOR_SWAP_WORD_SHIFT     1
#define XOR_SWAP_DWORD_SHIFT    0

#define XOR_SWAP_BYTE   4
#define XOR_SWAP_WORD   2
#define XOR_SWAP_DWORD  1

#define FORMAT_RGBA             0
#define FORMAT_YUV              1
#define FORMAT_CI               2
#define FORMAT_IA               3
#define FORMAT_I                4

#ifdef LSB_FIRST
#define BYTE_XOR_DWORD_SWAP 7
#define WORD_XOR_DWORD_SWAP 3
#else
#define BYTE_XOR_DWORD_SWAP 4
#define WORD_XOR_DWORD_SWAP 2
#endif
#define DWORD_XOR_DWORD_SWAP 1

#define GET_LOW_RGBA16_TMEM(x)  (m_rdp->m_replicated_rgba[((x) >> 1) & 0x1f])
#define GET_MED_RGBA16_TMEM(x)  (m_rdp->m_replicated_rgba[((x) >> 6) & 0x1f])
#define GET_HI_RGBA16_TMEM(x)   (m_rdp->m_replicated_rgba[((x) >> 11) & 0x1f])

#define MEM8_LIMIT  0x7fffff
#define MEM16_LIMIT 0x3fffff
#define MEM32_LIMIT 0x1fffff

#define RDP_RANGE_CHECK (0)

#if RDP_RANGE_CHECK
#define CHECK8(in) if(rdp_range_check((in))) { printf("Check8: Address %08x out of range!\n", (in)); fflush(stdout); fatalerror("Address %08x out of range!\n", (in)); }
#define CHECK16(in) if(rdp_range_check((in) << 1)) { printf("Check16: Address %08x out of range!\n", (in) << 1); fflush(stdout); fatalerror("Address %08x out of range!\n", (in) << 1); }
#define CHECK32(in) if(rdp_range_check((in) << 2)) { printf("Check32: Address %08x out of range!\n", (in) << 2); fflush(stdout); fatalerror("Address %08x out of range!\n", (in) << 2); }
#else
#define CHECK8(in) { }
#define CHECK16(in) { }
#define CHECK32(in) { }
#endif

#if RDP_RANGE_CHECK
#define RREADADDR8(in) ((rdp_range_check((in))) ? 0 : (((uint8_t*)m_rdram)[(in) ^ BYTE_ADDR_XOR]))
#define RREADIDX16(in) ((rdp_range_check((in) << 1)) ? 0 : (((uint16_t*)m_rdram)[(in) ^ WORD_ADDR_XOR]))
#define RREADIDX32(in) ((rdp_range_check((in) << 2)) ? 0 : m_rdram[(in)])

#define RWRITEADDR8(in, val)    if(rdp_range_check((in))) { printf("Write8: Address %08x out of range!\n", (in)); fflush(stdout); fatalerror("Address %08x out of range!\n", (in)); } else { ((uint8_t*)m_rdram)[(in) ^ BYTE_ADDR_XOR] = val;}
#define RWRITEIDX16(in, val)    if(rdp_range_check((in) << 1)) { printf("Write16: Address %08x out of range!\n", ((object.m_misc_state.m_fb_address >> 1) + curpixel) << 1); fflush(stdout); fatalerror("Address out of range\n"); } else { ((uint16_t*)m_rdram)[(in) ^ WORD_ADDR_XOR] = val;}
#define RWRITEIDX32(in, val)    if(rdp_range_check((in) << 2)) { printf("Write32: Address %08x out of range!\n", (in) << 2); fflush(stdout); fatalerror("Address %08x out of range!\n", (in) << 2); } else { m_rdram[(in)] = val;}
#else
#define RREADADDR8(in) (((uint8_t*)m_rdram)[(in) ^ BYTE_ADDR_XOR])
#define RREADIDX16(in) (((uint16_t*)m_rdram)[(in) ^ WORD_ADDR_XOR])
#define RREADIDX32(in) (m_rdram[(in)])

#define RWRITEADDR8(in, val)    ((uint8_t*)m_rdram)[(in) ^ BYTE_ADDR_XOR] = val;
#define RWRITEIDX16(in, val)    ((uint16_t*)m_rdram)[(in) ^ WORD_ADDR_XOR] = val;
#define RWRITEIDX32(in, val)    m_rdram[(in)] = val
#endif

#define U_RREADADDR8(in) (((uint8_t*)m_rdram)[(in) ^ BYTE_ADDR_XOR])
#define U_RREADIDX16(in) (((uint16_t*)m_rdram)[(in) ^ WORD_ADDR_XOR])
#define U_RREADIDX32(in) (m_rdram[(in)])

#define GETLOWCOL(x)    (((x) & 0x3e) << 2)
#define GETMEDCOL(x)    (((x) & 0x7c0) >> 3)
#define GETHICOL(x)     (((x) & 0xf800) >> 8)

#define HREADADDR8(in)          /*(((in) <= MEM8_LIMIT) ? */(m_hidden_bits[(in) ^ BYTE_ADDR_XOR])/* : 0)*/
#define HWRITEADDR8(in, val)    /*{if ((in) <= MEM8_LIMIT) */m_hidden_bits[(in) ^ BYTE_ADDR_XOR] = val;/*}*/

//sign-extension macros
#define SIGN22(x)   (((x & 0x00200000) * 0x7ff) | (x & 0x1fffff))
#define SIGN17(x)   (((x & 0x00010000) * 0xffff) | (x & 0xffff))
#define SIGN16(x)   (((x & 0x00008000) * 0x1ffff) | (x & 0x7fff))
#define SIGN13(x)   (((x & 0x00001000) * 0xfffff) | (x & 0xfff))
#define SIGN11(x)   (((x & 0x00000400) * 0x3fffff) | (x & 0x3ff))
#define SIGN9(x)    (((x & 0x00000100) * 0xffffff) | (x & 0xff))
#define SIGN8(x)    (((x & 0x00000080) * 0x1ffffff) | (x & 0x7f))

#define KURT_AKELEY_SIGN9(x)    ((((x) & 0x180) == 0x180) ? ((x) | ~0x1ff) : ((x) & 0x1ff))

#define SPAN_R      (0)
#define SPAN_G      (1)
#define SPAN_B      (2)
#define SPAN_A      (3)
#define SPAN_S      (4)
#define SPAN_T      (5)
#define SPAN_W      (6)
#define SPAN_Z      (7)

#define EXTENT_AUX_COUNT            (sizeof(rdp_span_aux)*(480*192)) // Screen coverage *192, more or less

/*****************************************************************************/

class n64_periphs;
class n64_rdp;

#include "video/n64types.h"
#include "video/rdpblend.h"
#include "video/rdptpipe.h"

class n64_state;

class n64_rdp : public poly_manager<uint32_t, rdp_poly_state, 8>
{
public:
	n64_rdp(n64_state &state, uint32_t* rdram, uint32_t* dmem);

	running_machine &machine() const { assert(m_machine != nullptr); return *m_machine; }

	void init_internal_state()
	{
		m_tmem = std::make_unique<uint8_t[]>(0x1000);
		memset(m_tmem.get(), 0, 0x1000);

#if !defined(MAME_RDP)
		uint8_t* normpoint = machine().root_device().memregion("normpoint")->base();
		uint8_t* normslope = machine().root_device().memregion("normslope")->base();

		for(int32_t i = 0; i < 64; i++)
		{
			m_norm_point_rom[i] = (normpoint[(i << 1) + 1] << 8) | normpoint[i << 1];
			m_norm_slope_rom[i] = (normslope[(i << 1) + 1] << 8) | normslope[i << 1];
		}
#endif

		memset(m_tiles, 0, 8 * sizeof(n64_tile_t));
		memset(m_cmd_data, 0, sizeof(m_cmd_data));

		for (int32_t i = 0; i < 8; i++)
		{
			m_tiles[i].num = i;
			m_tiles[i].invmm = rgbaint_t(~0, ~0, ~0, ~0);
			m_tiles[i].invmask = rgbaint_t(~0, ~0, ~0, ~0);
		}
	}

	void        process_command_list();
	uint64_t    read_data(uint32_t address);
	void        disassemble(uint64_t *cmd_buf, char* buffer);

	void        set_machine(running_machine& machine) { m_machine = &machine; }
	void        set_n64_periphs(n64_periphs* periphs) { m_n64_periphs = periphs; }

	// CPU-visible registers
	void        set_start(uint32_t val) { m_start = val; }
	uint32_t    get_start() const { return m_start; }

	void        set_end(uint32_t val) { m_end = val; }
	uint32_t    get_end() const { return m_end; }

	void        set_current(uint32_t val) { m_current = val; }
	uint32_t    get_current() const { return m_current; }

	void        set_status(uint32_t val) { m_status = val; }
	uint32_t    get_status() const { return m_status; }

	// Color Combiner
	int32_t       color_combiner_equation(int32_t a, int32_t b, int32_t c, int32_t d);
	int32_t       alpha_combiner_equation(int32_t a, int32_t b, int32_t c, int32_t d);
	void        set_suba_input_rgb(color_t** input, int32_t code, rdp_span_aux* userdata);
	void        set_subb_input_rgb(color_t** input, int32_t code, rdp_span_aux* userdata);
	void        set_mul_input_rgb(color_t** input, int32_t code, rdp_span_aux* userdata);
	void        set_add_input_rgb(color_t** input, int32_t code, rdp_span_aux* userdata);
	void        set_sub_input_alpha(color_t** input, int32_t code, rdp_span_aux* userdata);
	void        set_mul_input_alpha(color_t** input, int32_t code, rdp_span_aux* userdata);

	// Texture memory
	uint8_t*      get_tmem8() { return m_tmem.get(); }
	uint16_t*     get_tmem16() { return (uint16_t*)m_tmem.get(); }

	// YUV Factors
	void        set_yuv_factors(color_t k02, color_t k13, color_t k4, color_t k5) { m_k02 = k02; m_k13 = k13; m_k4 = k4; m_k5 = k5; }
	color_t&    get_k02() { return m_k02; }
	color_t&    get_k13() { return m_k13; }

	// Blender-related (move into RDP::Blender)
	void        set_blender_input(int32_t cycle, int32_t which, color_t** input_rgb, color_t** input_a, int32_t a, int32_t b, rdp_span_aux* userdata);

	// Span rasterization
	void        span_draw_1cycle(int32_t scanline, const extent_t &extent, const rdp_poly_state &object, int32_t threadid);
	void        span_draw_2cycle(int32_t scanline, const extent_t &extent, const rdp_poly_state &object, int32_t threadid);
	void        span_draw_copy(int32_t scanline, const extent_t &extent, const rdp_poly_state &object, int32_t threadid);
	void        span_draw_fill(int32_t scanline, const extent_t &extent, const rdp_poly_state &object, int32_t threadid);

	// Render-related (move into eventual drawing-related classes?)
	void            tc_div(int32_t ss, int32_t st, int32_t sw, int32_t* sss, int32_t* sst);
	void            tc_div_no_perspective(int32_t ss, int32_t st, int32_t sw, int32_t* sss, int32_t* sst);
	uint32_t          get_log2(uint32_t lod_clamp);
	void            render_spans(int32_t start, int32_t end, int32_t tilenum, bool flip, extent_t* spans, bool rect, rdp_poly_state* object);
	int32_t           get_alpha_cvg(int32_t comb_alpha, rdp_span_aux* userdata, const rdp_poly_state &object);

	void            z_store(const rdp_poly_state &object, uint32_t zcurpixel, uint32_t dzcurpixel, uint32_t z, uint32_t enc);
	uint32_t          z_decompress(uint32_t zcurpixel);
	uint32_t          dz_decompress(uint32_t zcurpixel, uint32_t dzcurpixel);
	uint32_t          dz_compress(uint32_t value);
	int32_t           normalize_dzpix(int32_t sum);
	bool            z_compare(uint32_t zcurpixel, uint32_t dzcurpixel, uint32_t sz, uint16_t dzpix, rdp_span_aux* userdata, const rdp_poly_state &object);

	// Commands
	void        cmd_noop(uint64_t *cmd_buf);
	void        cmd_tex_rect(uint64_t *cmd_buf);
	void        cmd_tex_rect_flip(uint64_t *cmd_buf);
	void        cmd_sync_load(uint64_t *cmd_buf);
	void        cmd_sync_pipe(uint64_t *cmd_buf);
	void        cmd_sync_tile(uint64_t *cmd_buf);
	void        cmd_sync_full(uint64_t *cmd_buf);
	void        cmd_set_key_gb(uint64_t *cmd_buf);
	void        cmd_set_key_r(uint64_t *cmd_buf);
	void        cmd_set_fill_color32(uint64_t *cmd_buf);
	void        cmd_set_convert(uint64_t *cmd_buf);
	void        cmd_set_scissor(uint64_t *cmd_buf);
	void        cmd_set_prim_depth(uint64_t *cmd_buf);
	void        cmd_set_other_modes(uint64_t *cmd_buf);
	void        cmd_load_tlut(uint64_t *cmd_buf);
	void        cmd_set_tile_size(uint64_t *cmd_buf);
	void        cmd_load_block(uint64_t *cmd_buf);
	void        cmd_load_tile(uint64_t *cmd_buf);
	void        cmd_fill_rect(uint64_t *cmd_buf);
	void        cmd_set_tile(uint64_t *cmd_buf);
	void        cmd_set_fog_color(uint64_t *cmd_buf);
	void        cmd_set_blend_color(uint64_t *cmd_buf);
	void        cmd_set_prim_color(uint64_t *cmd_buf);
	void        cmd_set_env_color(uint64_t *cmd_buf);
	void        cmd_set_combine(uint64_t *cmd_buf);
	void        cmd_set_texture_image(uint64_t *cmd_buf);
	void        cmd_set_mask_image(uint64_t *cmd_buf);
	void        cmd_set_color_image(uint64_t *cmd_buf);

	void        rgbaz_clip(int32_t sr, int32_t sg, int32_t sb, int32_t sa, int32_t* sz, rdp_span_aux* userdata);
	void        rgbaz_correct_triangle(int32_t offx, int32_t offy, int32_t* r, int32_t* g, int32_t* b, int32_t* a, int32_t* z, rdp_span_aux* userdata, const rdp_poly_state &object);

	void        triangle(uint64_t *cmd_buf, bool shade, bool texture, bool zbuffer);

	void        get_dither_values(int32_t x, int32_t y, int32_t* cdith, int32_t* adith, const rdp_poly_state &object);

	uint16_t decompress_cvmask_frombyte(uint8_t x);
	void lookup_cvmask_derivatives(uint32_t mask, uint8_t* offx, uint8_t* offy, rdp_span_aux* userdata);

	void        mark_frame() { m_capture.mark_frame(*m_machine); }

	misc_state_t m_misc_state;

	// Color constants
	color_t         m_blend_color;          /* constant blend color */
	color_t         m_prim_color;           /* flat primitive color */
	color_t         m_prim_alpha;           /* flat primitive alpha */
	color_t         m_env_color;            /* generic color constant ('environment') */
	color_t         m_env_alpha;            /* generic alpha constant ('environment') */
	color_t         m_fog_color;            /* generic color constant ('fog') */
	color_t         m_key_scale;            /* color-keying constant */
	color_t         m_lod_fraction;         /* Z-based LOD fraction for this poly */
	color_t         m_prim_lod_fraction;    /* fixed LOD fraction for this poly */

	color_t         m_one;
	color_t         m_zero;

	uint32_t          m_fill_color;

	other_modes_t   m_other_modes;

	n64_blender_t   m_blender;

	n64_texture_pipe_t m_tex_pipe;

	uint8_t m_hidden_bits[0x800000];

	uint8_t m_replicated_rgba[32];

	uint16_t m_dzpix_normalize[0x10000];

	rectangle_t     m_scissor;
	span_base_t     m_span_base;

	void            draw_triangle(uint64_t *cmd_buf, bool shade, bool texture, bool zbuffer, bool rect);

	std::unique_ptr<uint8_t[]>  m_aux_buf;
	uint32_t          m_aux_buf_ptr;
	uint32_t          m_aux_buf_index;

	bool            rdp_range_check(uint32_t addr);

	n64_tile_t      m_tiles[8];

private:
	void    compute_cvg_noflip(extent_t* spans, int32_t* majorx, int32_t* minorx, int32_t* majorxint, int32_t* minorxint, int32_t scanline, int32_t yh, int32_t yl, int32_t base);
	void    compute_cvg_flip(extent_t* spans, int32_t* majorx, int32_t* minorx, int32_t* majorxint, int32_t* minorxint, int32_t scanline, int32_t yh, int32_t yl, int32_t base);

	void    write_pixel4(uint32_t curpixel, color_t& color, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    write_pixel8(uint32_t curpixel, color_t& color, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    write_pixel16(uint32_t curpixel, color_t& color, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    write_pixel32(uint32_t curpixel, color_t& color, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    read_pixel4(uint32_t curpixel, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    read_pixel8(uint32_t curpixel, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    read_pixel16(uint32_t curpixel, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    read_pixel32(uint32_t curpixel, rdp_span_aux* userdata, const rdp_poly_state &object);
	void    copy_pixel4(uint32_t curpixel, color_t& color, const rdp_poly_state &object);
	void    copy_pixel8(uint32_t curpixel, color_t& color, const rdp_poly_state &object);
	void    copy_pixel16(uint32_t curpixel, color_t& color, const rdp_poly_state &object);
	void    copy_pixel32(uint32_t curpixel, color_t& color, const rdp_poly_state &object);
	void    fill_pixel4(uint32_t curpixel, const rdp_poly_state &object);
	void    fill_pixel8(uint32_t curpixel, const rdp_poly_state &object);
	void    fill_pixel16(uint32_t curpixel, const rdp_poly_state &object);
	void    fill_pixel32(uint32_t curpixel, const rdp_poly_state &object);

	void    precalc_cvmask_derivatives(void);
	void    z_build_com_table(void);

	typedef void (n64_rdp::*compute_cvg_t) (extent_t* spans, int32_t* majorx, int32_t* minorx, int32_t* majorxint, int32_t* minorxint, int32_t scanline, int32_t yh, int32_t yl, int32_t base);
	compute_cvg_t     m_compute_cvg[2];

	typedef void (n64_rdp::*write_pixel_t) (uint32_t curpixel, color_t& color, rdp_span_aux* userdata, const rdp_poly_state &object);
	typedef void (n64_rdp::*read_pixel_t) (uint32_t curpixel, rdp_span_aux* userdata, const rdp_poly_state &object);
	typedef void (n64_rdp::*copy_pixel_t) (uint32_t curpixel, color_t& color, const rdp_poly_state &object);
	typedef void (n64_rdp::*fill_pixel_t) (uint32_t curpixel, const rdp_poly_state &object);

	write_pixel_t     m_write_pixel[4];
	read_pixel_t      m_read_pixel[4];
	copy_pixel_t      m_copy_pixel[4];
	fill_pixel_t      m_fill_pixel[4];

	running_machine*  m_machine;
	uint32_t*         m_rdram;
	uint32_t*         m_dmem;
	n64_periphs* m_n64_periphs;

	combine_modes_t m_combine;
	bool            m_pending_mode_block;
	bool            m_pipe_clean;

	cv_mask_derivative_t cvarray[(1 << 8)];

	uint16_t  m_z_com_table[0x40000]; //precalced table of compressed z values, 18b: 512 KB array!
	uint32_t  m_z_complete_dec_table[0x4000]; //the same for decompressed z values, 14b
	uint8_t   m_compressed_cvmasks[0x10000]; //16bit cvmask -> to byte

	uint64_t  m_cmd_data[0x800];
	uint64_t  m_temp_rect_data[0x800];

	uint32_t  m_start;
	uint32_t  m_end;
	uint32_t  m_current;
	uint32_t  m_status;

	std::unique_ptr<uint8_t[]>  m_tmem;

	// YUV factors
	color_t m_k02;
	color_t m_k13;
	color_t m_k4;
	color_t m_k5;

	// Texture perspective division
#if !defined(MAME_RDP)
	int32_t m_norm_point_rom[64];
	int32_t m_norm_slope_rom[64];
#else
	int32_t m_norm_point_rom[64] =
	{
		0x4000, 0x3f04, 0x3e10, 0x3d22, 0x3c3c, 0x3b5d, 0x3a83, 0x39b1,
		0x38e4, 0x381c, 0x375a, 0x369d, 0x35e5, 0x3532, 0x3483, 0x33d9,
		0x3333, 0x3291, 0x31f4, 0x3159, 0x30c3, 0x3030, 0x2fa1, 0x2f15,
		0x2e8c, 0x2e06, 0x2d83, 0x2d03, 0x2c86, 0x2c0b, 0x2b93, 0x2b1e,
		0x2aab, 0x2a3a, 0x29cc, 0x2960, 0x28f6, 0x288e, 0x2828, 0x27c4,
		0x2762, 0x2702, 0x26a4, 0x2648, 0x25ed, 0x2594, 0x253d, 0x24e7,
		0x2492, 0x243f, 0x23ee, 0x239e, 0x234f, 0x2302, 0x22b6, 0x226c,
		0x2222, 0x21da, 0x2193, 0x214d, 0x2108, 0x20c5, 0x2082, 0x2041,
	};
	int32_t m_norm_slope_rom[64] =
	{
		0xfc, 0xf4, 0xee, 0xe6, 0xdf, 0xda, 0xd2, 0xcd,
		0xc8, 0xc2, 0xbd, 0xb8, 0xb3, 0xaf, 0xaa, 0xa6,
		0xa2, 0x9d, 0x9b, 0x96, 0x93, 0x8f, 0x8c, 0x89,
		0x86, 0x83, 0x80, 0x7d, 0x7b, 0x78, 0x75, 0x73,
		0x71, 0x6e, 0x6c, 0x6a, 0x68, 0x66, 0x64, 0x62,
		0x60, 0x5e, 0x5c, 0x5b, 0x59, 0x57, 0x56, 0x55,
		0x53, 0x51, 0x50, 0x4f, 0x4d, 0x4c, 0x4a, 0x4a,
		0x48, 0x47, 0x46, 0x45, 0x43, 0x43, 0x41, 0x41,
	};
#endif

	pin64_t m_capture;

	static uint32_t s_special_9bit_clamptable[512];
	static z_decompress_entry_t const m_z_dec_table[8];

	static uint8_t const s_bayer_matrix[16];
	static uint8_t const s_magic_matrix[16];
	static int32_t const s_rdp_command_length[];
	static char const *const s_image_format[];
	static char const *const s_image_size[];

public:
	bool ignore;
	bool dolog;
};

#endif // _VIDEO_N64_H_
