// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    rgbutil.h

    Utility definitions for RGB manipulation. Allows RGB handling to be
    performed in an abstracted fashion and optimized with SIMD.

***************************************************************************/

#ifndef MAME_EMU_VIDEO_RGBUTIL_H
#define MAME_EMU_VIDEO_RGBUTIL_H

// use SSE on 64-bit implementations, where it can be assumed
#if (!defined(MAME_DEBUG) || defined(__OPTIMIZE__)) && (defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)))

#define MAME_RGB_HIGH_PRECISION
#include "rgbsse.h"
#ifdef MAME_RGBUTIL_IMPL
#include "rgbsse.cpp"
#endif

#elif defined(__aarch64__)

#define MAME_RGB_HIGH_PRECISION
#include "rgbsse.h"
#ifdef MAME_RGBUTIL_IMPL
#include "rgbsse.cpp"
#endif

#elif defined(__ALTIVEC__)

#define MAME_RGB_HIGH_PRECISION
#include "rgbvmx.h"
#ifdef MAME_RGBUTIL_IMPL
#include "rgbvmx.cpp"
#endif

#else

#include "rgbgen.h"
#ifdef MAME_RGBUTIL_IMPL
#include "rgbgen.cpp"
#endif

#endif

#endif // MAME_EMU_VIDEO_RGBUTIL_H
