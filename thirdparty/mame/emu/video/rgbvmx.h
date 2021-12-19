// license:BSD-3-Clause
// copyright-holders:Vas Crabb, Ryan Holtz
/***************************************************************************

    rgbvmx.h

    VMX/Altivec optimised RGB utilities.

***************************************************************************/

#ifndef MAME_EMU_VIDEO_RGBVMX_H
#define MAME_EMU_VIDEO_RGBVMX_H

#pragma once

#include <altivec.h>

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

class rgbaint_t
{
protected:
	typedef __vector signed char    VECS8;
	typedef __vector unsigned char  VECU8;
	typedef __vector signed short   VECS16;
	typedef __vector unsigned short VECU16;
	typedef __vector signed int     VECS32;
	typedef __vector unsigned int   VECU32;

public:
	rgbaint_t() { set(0, 0, 0, 0); }
	explicit rgbaint_t(u32 rgba) { set(rgba); }
	rgbaint_t(s32 a, s32 r, s32 g, s32 b) { set(a, r, g, b); }
	explicit rgbaint_t(const rgb_t& rgb) { set(rgb); }
	explicit rgbaint_t(VECS32 rgba) : m_value(rgba) { }

	rgbaint_t(const rgbaint_t& other) = default;
	rgbaint_t &operator=(const rgbaint_t& other) = default;

	void set(const rgbaint_t& other) { m_value = other.m_value; }

	void set(u32 rgba)
	{
		const VECU32 zero = { 0, 0, 0, 0 };
#ifdef __LITTLE_ENDIAN__
		const VECS8 temp = *reinterpret_cast<const VECS8 *>(&rgba);
		m_value = VECS32(vec_mergeh(VECS16(vec_mergeh(temp, VECS8(zero))), VECS16(zero)));
#else
		const VECS8 temp = VECS8(vec_perm(vec_lde(0, &rgba), zero, vec_lvsl(0, &rgba)));
		m_value = VECS32(vec_mergeh(VECS16(zero), VECS16(vec_mergeh(VECS8(zero), temp))));
#endif
	}

	void set(s32 a, s32 r, s32 g, s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 result = { b, g, r, a };
#else
		const VECS32 result = { a, r, g, b };
#endif
		m_value = result;
	}

	void set(const rgb_t& rgb)
	{
		const VECU32 zero = { 0, 0, 0, 0 };
#ifdef __LITTLE_ENDIAN__
		const VECS8 temp = *reinterpret_cast<const VECS8 *>(rgb.ptr());
		m_value = VECS32(vec_mergeh(VECS16(vec_mergeh(temp, VECS8(zero))), VECS16(zero)));
#else
		const VECS8 temp = VECS8(vec_perm(vec_lde(0, rgb.ptr()), zero, vec_lvsl(0, rgb.ptr())));
		m_value = VECS32(vec_mergeh(VECS16(zero), VECS16(vec_mergeh(VECS8(zero), temp))));
#endif
	}

	// This function sets all elements to the same val
	void set_all(const s32& val) { set(val, val, val, val); }
	// This function zeros all elements
	void zero() { set_all(0); }
	// This function zeros only the alpha element
	void zero_alpha() { set_a(0); }

	inline rgb_t to_rgba() const
	{
		VECU32 temp = VECU32(vec_packs(m_value, m_value));
		temp = VECU32(vec_packsu(VECS16(temp), VECS16(temp)));
		u32 result;
		vec_ste(temp, 0, &result);
		return result;
	}

	inline rgb_t to_rgba_clamp() const
	{
		VECU32 temp = VECU32(vec_packs(m_value, m_value));
		temp = VECU32(vec_packsu(VECS16(temp), VECS16(temp)));
		u32 result;
		vec_ste(temp, 0, &result);
		return result;
	}

	void set_a16(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_perm(m_value, temp, alpha_perm);
	}

	void set_a(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_perm(m_value, temp, alpha_perm);
	}

	void set_r(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_perm(m_value, temp, red_perm);
	}

	void set_g(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_perm(m_value, temp, green_perm);
	}

	void set_b(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_perm(m_value, temp, blue_perm);
	}

	u8 get_a() const
	{
		u8 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(VECU8(m_value), 12), 0, &result);
#else
		vec_ste(vec_splat(VECU8(m_value), 3), 0, &result);
#endif
		return result;
	}

	u8 get_r() const
	{
		u8 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(VECU8(m_value), 8), 0, &result);
#else
		vec_ste(vec_splat(VECU8(m_value), 7), 0, &result);
#endif
		return result;
	}

	u8 get_g() const
	{
		u8 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(VECU8(m_value), 4), 0, &result);
#else
		vec_ste(vec_splat(VECU8(m_value), 11), 0, &result);
#endif
		return result;
	}

	u8 get_b() const
	{
		u8 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(VECU8(m_value), 0), 0, &result);
#else
		vec_ste(vec_splat(VECU8(m_value), 15), 0, &result);
#endif
		return result;
	}

	s32 get_a32() const
	{
		s32 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(m_value, 3), 0, &result);
#else
		vec_ste(vec_splat(m_value, 0), 0, &result);
#endif
		return result;
	}

	s32 get_r32() const
	{
		s32 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(m_value, 2), 0, &result);
#else
		vec_ste(vec_splat(m_value, 1), 0, &result);
#endif
		return result;
	}

	s32 get_g32() const
	{
		s32 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(m_value, 1), 0, &result);
#else
		vec_ste(vec_splat(m_value, 2), 0, &result);
#endif
		return result;
	}

	s32 get_b32() const
	{
		s32 result;
#ifdef __LITTLE_ENDIAN__
		vec_ste(vec_splat(m_value, 0), 0, &result);
#else
		vec_ste(vec_splat(m_value, 3), 0, &result);
#endif
		return result;
	}

	// These selects return an rgbaint_t with all fields set to the element choosen (a, r, g, or b)
	rgbaint_t select_alpha32() const { return rgbaint_t(get_a32(), get_a32(), get_a32(), get_a32()); }
	rgbaint_t select_red32() const { return rgbaint_t(get_r32(), get_r32(), get_r32(), get_r32()); }
	rgbaint_t select_green32() const { return rgbaint_t(get_g32(), get_g32(), get_g32(), get_g32()); }
	rgbaint_t select_blue32() const { return rgbaint_t(get_b32(), get_b32(), get_b32(), get_b32()); }

	inline void add(const rgbaint_t& color2)
	{
		m_value = vec_add(m_value, color2.m_value);
	}

	inline void add_imm(const s32 imm)
	{
		const VECS32 temp = { imm, imm, imm, imm };
		m_value = vec_add(m_value, temp);
	}

	inline void add_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = vec_add(m_value, temp);
	}

	inline void sub(const rgbaint_t& color2)
	{
		m_value = vec_sub(m_value, color2.m_value);
	}

	inline void sub_imm(const s32 imm)
	{
		const VECS32 temp = { imm, imm, imm, imm };
		m_value = vec_sub(m_value, temp);
	}

	inline void sub_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = vec_sub(m_value, temp);
	}

	inline void subr(const rgbaint_t& color2)
	{
		m_value = vec_sub(color2.m_value, m_value);
	}

	inline void subr_imm(const s32 imm)
	{
		const VECS32 temp = { imm, imm, imm, imm };
		m_value = vec_sub(temp, m_value);
	}

	inline void subr_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = vec_sub(temp, m_value);
	}

	inline void mul(const rgbaint_t& color)
	{
		const VECU32 shift = vec_splat_u32(-16);
		const VECU32 temp = vec_msum(VECU16(m_value), VECU16(vec_rl(color.m_value, shift)), vec_splat_u32(0));
#ifdef __LITTLE_ENDIAN__
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mule(VECU16(m_value), VECU16(color.m_value))));
#else
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mulo(VECU16(m_value), VECU16(color.m_value))));
#endif
	}

	inline void mul_imm(const s32 imm)
	{
		const VECU32 value = { u32(imm), u32(imm), u32(imm), u32(imm) };
		const VECU32 shift = vec_splat_u32(-16);
		const VECU32 temp = vec_msum(VECU16(m_value), VECU16(vec_rl(value, shift)), vec_splat_u32(0));
#ifdef __LITTLE_ENDIAN__
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mule(VECU16(m_value), VECU16(value))));
#else
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mulo(VECU16(m_value), VECU16(value))));
#endif
	}

	inline void mul_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECU32 value = { u32(b), u32(g), u32(r), u32(a) };
#else
		const VECU32 value = { u32(a), u32(r), u32(g), u32(b) };
#endif
		const VECU32 shift = vec_splat_u32(-16);
		const VECU32 temp = vec_msum(VECU16(m_value), VECU16(vec_rl(value, shift)), vec_splat_u32(0));
#ifdef __LITTLE_ENDIAN__
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mule(VECU16(m_value), VECU16(value))));
#else
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mulo(VECU16(m_value), VECU16(value))));
#endif
	}

	inline void shl(const rgbaint_t& shift)
	{
		const VECU32 limit = { 32, 32, 32, 32 };
		m_value = vec_and(vec_sl(m_value, VECU32(shift.m_value)), vec_cmpgt(limit, VECU32(shift.m_value)));
	}

	inline void shl_imm(const u8 shift)
	{
		const VECU32 temp = { shift, shift, shift, shift };
		m_value = vec_sl(m_value, temp);
	}

	inline void shr(const rgbaint_t& shift)
	{
		const VECU32 limit = { 32, 32, 32, 32 };
		m_value = vec_and(vec_sr(m_value, VECU32(shift.m_value)), vec_cmpgt(limit, VECU32(shift.m_value)));
	}

	inline void shr_imm(const u8 shift)
	{
		const VECU32 temp = { shift, shift, shift, shift };
		m_value = vec_sr(m_value, temp);
	}

	inline void sra(const rgbaint_t& shift)
	{
		const VECU32 limit = { 31, 31, 31, 31 };
		m_value = vec_sra(m_value, vec_min(VECU32(shift.m_value), limit));
	}

	inline void sra_imm(const u8 shift)
	{
		const VECU32 temp = { shift, shift, shift, shift };
		m_value = vec_sra(m_value, temp);
	}

	inline void or_reg(const rgbaint_t& color2)
	{
		m_value = vec_or(m_value, color2.m_value);
	}

	inline void or_imm(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_or(m_value, temp);
	}

	inline void or_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = vec_or(m_value, temp);
	}

	inline void and_reg(const rgbaint_t& color)
	{
		m_value = vec_and(m_value, color.m_value);
	}

	inline void andnot_reg(const rgbaint_t& color)
	{
		m_value = vec_andc(m_value, color.m_value);
	}

	inline void and_imm(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_and(m_value, temp);
	}

	inline void and_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = vec_and(m_value, temp);
	}

	inline void xor_reg(const rgbaint_t& color2)
	{
		m_value = vec_xor(m_value, color2.m_value);
	}

	inline void xor_imm(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_xor(m_value, temp);
	}

	inline void xor_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = vec_xor(m_value, temp);
	}

	inline void clamp_and_clear(const u32 sign)
	{
		const VECS32 vzero = { 0, 0, 0, 0 };
		VECS32 vsign = { s32(sign), s32(sign), s32(sign), s32(sign) };
		m_value = vec_and(m_value, vec_cmpeq(vec_and(m_value, vsign), vzero));
		vsign = vec_nor(vec_sra(vsign, vec_splat_u32(1)), vzero);
		const VECS32 mask = VECS32(vec_cmpgt(m_value, vsign));
		m_value = vec_or(vec_and(vsign, mask), vec_and(m_value, vec_nor(mask, vzero)));
	}

	inline void clamp_to_uint8()
	{
		const VECU32 zero = { 0, 0, 0, 0 };
		m_value = VECS32(vec_packs(m_value, m_value));
		m_value = VECS32(vec_packsu(VECS16(m_value), VECS16(m_value)));
#ifdef __LITTLE_ENDIAN__
		m_value = VECS32(vec_mergeh(VECU8(m_value), VECU8(zero)));
		m_value = VECS32(vec_mergeh(VECS16(m_value), VECS16(zero)));
#else
		m_value = VECS32(vec_mergeh(VECU8(zero), VECU8(m_value)));
		m_value = VECS32(vec_mergeh(VECS16(zero), VECS16(m_value)));
#endif
	}

	inline void sign_extend(const u32 compare, const u32 sign)
	{
		const VECS32 compare_vec = { s32(compare), s32(compare), s32(compare), s32(compare) };
		const VECS32 compare_mask = VECS32(vec_cmpeq(vec_and(m_value, compare_vec), compare_vec));
		const VECS32 sign_vec = { s32(sign), s32(sign), s32(sign), s32(sign) };
		m_value = vec_or(m_value, vec_and(sign_vec, compare_mask));
	}

	inline void min(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_min(m_value, temp);
	}

	inline void max(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = vec_max(m_value, temp);
	}

	void blend(const rgbaint_t& other, u8 factor);

	void scale_and_clamp(const rgbaint_t& scale);
	void scale_imm_and_clamp(const s32 scale);

	void scale_add_and_clamp(const rgbaint_t& scale, const rgbaint_t& other)
	{
		mul(scale);
		sra_imm(8);
		add(other);
		clamp_to_uint8();
	}

	void scale2_add_and_clamp(const rgbaint_t& scale, const rgbaint_t& other, const rgbaint_t& scale2)
	{
		rgbaint_t color2(other);
		color2.mul(scale2);

		mul(scale);
		add(color2);
		sra_imm(8);
		clamp_to_uint8();
	}

	inline void cmpeq(const rgbaint_t& value)
	{
		m_value = VECS32(vec_cmpeq(m_value, value.m_value));
	}

	inline void cmpeq_imm(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = VECS32(vec_cmpeq(m_value, temp));
	}

	inline void cmpeq_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = VECS32(vec_cmpeq(m_value, temp));
	}

	inline void cmpgt(const rgbaint_t& value)
	{
		m_value = VECS32(vec_cmpgt(m_value, value.m_value));
	}

	inline void cmpgt_imm(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = VECS32(vec_cmpgt(m_value, temp));
	}

	inline void cmpgt_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = VECS32(vec_cmpgt(m_value, temp));
	}

	inline void cmplt(const rgbaint_t& value)
	{
		m_value = VECS32(vec_cmplt(m_value, value.m_value));
	}

	inline void cmplt_imm(const s32 value)
	{
		const VECS32 temp = { value, value, value, value };
		m_value = VECS32(vec_cmplt(m_value, temp));
	}

	inline void cmplt_imm_rgba(const s32 a, const s32 r, const s32 g, const s32 b)
	{
#ifdef __LITTLE_ENDIAN__
		const VECS32 temp = { b, g, r, a };
#else
		const VECS32 temp = { a, r, g, b };
#endif
		m_value = VECS32(vec_cmplt(m_value, temp));
	}

	inline rgbaint_t& operator+=(const rgbaint_t& other)
	{
		m_value = vec_add(m_value, other.m_value);
		return *this;
	}

	inline rgbaint_t& operator+=(const s32 other)
	{
		const VECS32 temp = { other, other, other, other };
		m_value = vec_add(m_value, temp);
		return *this;
	}

	inline rgbaint_t& operator-=(const rgbaint_t& other)
	{
		m_value = vec_sub(m_value, other.m_value);
		return *this;
	}

	inline rgbaint_t& operator*=(const rgbaint_t& other)
	{
		const VECU32 shift = vec_splat_u32(-16);
		const VECU32 temp = vec_msum(VECU16(m_value), VECU16(vec_rl(other.m_value, shift)), vec_splat_u32(0));
#ifdef __LITTLE_ENDIAN__
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mule(VECU16(m_value), VECU16(other.m_value))));
#else
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mulo(VECU16(m_value), VECU16(other.m_value))));
#endif
		return *this;
	}

	inline rgbaint_t& operator*=(const s32 other)
	{
		const VECS32 value = { other, other, other, other };
		const VECU32 shift = vec_splat_u32(-16);
		const VECU32 temp = vec_msum(VECU16(m_value), VECU16(vec_rl(value, shift)), vec_splat_u32(0));
#ifdef __LITTLE_ENDIAN__
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mule(VECU16(m_value), VECU16(value))));
#else
		m_value = VECS32(vec_add(vec_sl(temp, shift), vec_mulo(VECU16(m_value), VECU16(value))));
#endif
		return *this;
	}

	inline rgbaint_t& operator>>=(const s32 shift)
	{
		const VECU32 temp = { u32(shift), u32(shift), u32(shift), u32(shift) };
		m_value = vec_sra(m_value, temp);
		return *this;
	}

	inline void merge_alpha16(const rgbaint_t& alpha)
	{
		m_value = vec_perm(m_value, alpha.m_value, alpha_perm);
	}

	inline void merge_alpha(const rgbaint_t& alpha)
	{
		m_value = vec_perm(m_value, alpha.m_value, alpha_perm);
	}

	static u32 bilinear_filter(const u32 &rgb00, const u32 &rgb01, const u32 &rgb10, const u32 &rgb11, u8 u, u8 v)
	{
		const VECS32 zero = vec_splat_s32(0);

		// put each packed value into first element of a vector register
#ifdef __LITTLE_ENDIAN__
		VECS32 color00 = *reinterpret_cast<const VECS32 *>(&rgb00);
		VECS32 color01 = *reinterpret_cast<const VECS32 *>(&rgb01);
		VECS32 color10 = *reinterpret_cast<const VECS32 *>(&rgb10);
		VECS32 color11 = *reinterpret_cast<const VECS32 *>(&rgb11);
#else
		VECS32 color00 = vec_perm(VECS32(vec_lde(0, &rgb00)), zero, vec_lvsl(0, &rgb00));
		VECS32 color01 = vec_perm(VECS32(vec_lde(0, &rgb01)), zero, vec_lvsl(0, &rgb01));
		VECS32 color10 = vec_perm(VECS32(vec_lde(0, &rgb10)), zero, vec_lvsl(0, &rgb10));
		VECS32 color11 = vec_perm(VECS32(vec_lde(0, &rgb11)), zero, vec_lvsl(0, &rgb11));
#endif

		// interleave color01/color00 and color10/color11 at the byte level then zero-extend
		color01 = VECS32(vec_mergeh(VECU8(color01), VECU8(color00)));
		color11 = VECS32(vec_mergeh(VECU8(color11), VECU8(color10)));
#ifdef __LITTLE_ENDIAN__
		color01 = VECS32(vec_mergeh(VECU8(color01), VECU8(zero)));
		color11 = VECS32(vec_mergeh(VECU8(color11), VECU8(zero)));
#else
		color01 = VECS32(vec_mergeh(VECU8(zero), VECU8(color01)));
		color11 = VECS32(vec_mergeh(VECU8(zero), VECU8(color11)));
#endif

		color01 = vec_msum(VECS16(color01), scale_table[u], zero);
		color11 = vec_msum(VECS16(color11), scale_table[u], zero);
		color01 = vec_sl(color01, vec_splat_u32(15));
		color11 = vec_sr(color11, vec_splat_u32(1));
		color01 = VECS32(vec_max(VECS16(color01), VECS16(color11)));
		color01 = vec_msum(VECS16(color01), scale_table[v], zero);
		color01 = vec_sr(color01, vec_splat_u32(15));
		color01 = VECS32(vec_packs(color01, color01));
		color01 = VECS32(vec_packsu(VECS16(color01), VECS16(color01)));

		u32 result;
		vec_ste(VECU32(color01), 0, &result);
		return result;
	}

	void bilinear_filter_rgbaint(const u32 &rgb00, const u32 &rgb01, const u32 &rgb10, const u32 &rgb11, u8 u, u8 v)
	{
		const VECS32 zero = vec_splat_s32(0);

		// put each packed value into first element of a vector register
#ifdef __LITTLE_ENDIAN__
		VECS32 color00 = *reinterpret_cast<const VECS32 *>(&rgb00);
		VECS32 color01 = *reinterpret_cast<const VECS32 *>(&rgb01);
		VECS32 color10 = *reinterpret_cast<const VECS32 *>(&rgb10);
		VECS32 color11 = *reinterpret_cast<const VECS32 *>(&rgb11);
#else
		VECS32 color00 = vec_perm(VECS32(vec_lde(0, &rgb00)), zero, vec_lvsl(0, &rgb00));
		VECS32 color01 = vec_perm(VECS32(vec_lde(0, &rgb01)), zero, vec_lvsl(0, &rgb01));
		VECS32 color10 = vec_perm(VECS32(vec_lde(0, &rgb10)), zero, vec_lvsl(0, &rgb10));
		VECS32 color11 = vec_perm(VECS32(vec_lde(0, &rgb11)), zero, vec_lvsl(0, &rgb11));
#endif

		// interleave color01/color00 and color10/color11 at the byte level then zero-extend
		color01 = VECS32(vec_mergeh(VECU8(color01), VECU8(color00)));
		color11 = VECS32(vec_mergeh(VECU8(color11), VECU8(color10)));
#ifdef __LITTLE_ENDIAN__
		color01 = VECS32(vec_mergeh(VECU8(color01), VECU8(zero)));
		color11 = VECS32(vec_mergeh(VECU8(color11), VECU8(zero)));
#else
		color01 = VECS32(vec_mergeh(VECU8(zero), VECU8(color01)));
		color11 = VECS32(vec_mergeh(VECU8(zero), VECU8(color11)));
#endif

		color01 = vec_msum(VECS16(color01), scale_table[u], zero);
		color11 = vec_msum(VECS16(color11), scale_table[u], zero);
		color01 = vec_sl(color01, vec_splat_u32(15));
		color11 = vec_sr(color11, vec_splat_u32(1));
		color01 = VECS32(vec_max(VECS16(color01), VECS16(color11)));
		color01 = vec_msum(VECS16(color01), scale_table[v], zero);
		m_value = vec_sr(color01, vec_splat_u32(15));
	}

protected:
	VECS32              m_value;

	static const VECU8  alpha_perm;
	static const VECU8  red_perm;
	static const VECU8  green_perm;
	static const VECU8  blue_perm;
	static const VECS16 scale_table[256];
};



// altivec.h somehow redefines "bool" in a bad way.  really.
#ifdef vector
#undef vector
#endif
#ifdef bool
#undef bool
#endif
#ifdef pixel
#undef pixel
#endif

#endif // MAME_EMU_VIDEO_RGBVMX_H
