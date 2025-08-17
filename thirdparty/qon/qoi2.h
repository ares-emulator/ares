/*

Copyright (c) 2025, Roger Sanders
Revised QOI format, achieving higher compression with equivalent performance.

Based on the "Quite OK Image" format for fast, lossless image compression
Copyright (c) 2021, Dominic Szablewski - https://phoboslab.org

SPDX-License-Identifier: MIT


-- About

QOI encodes and decodes images in a lossless format. Compared to stb_image and
stb_image_write QOI offers 20x-50x faster encoding, 3x-4x faster decoding and
20% better compression.

QOI2 improves on the original QOI compression rates by optimising encoding for
both most compressible and least compressible data.


-- Format changes

The original QOI bit encoding scheme is as follows:
   QOI_OP_INDEX: 00 IIIIII = Color table entry I
   QOI_OP_DIFF:  01 RRGGBB = Diff from previous pixel, -2 to 1
   QOI_OP_LUMA:  10 GGGGGG RRRR BBBB = Diff from previous pixel, G -32 to 31, R and B diff to green diff -8 to 7
   QOI_OP_RUN:   11 YYYYYY = Repeat previous pixel Y+1 times, up to 62.
   QOI_OP_RGB:   11111110  RRRRRRRR GGGGGGGG BBBBBBBB = Literal RGB value
   QOI_OP_RGBA:  11111111  RRRRRRRR GGGGGGGG BBBBBBBB AAAAAAAA = Literal RGBA value

The improved QOI2 bit encoding scheme is as follows:
   QOI2_OP_INDEX:  00 IIIIII = Color table entry I
   QOI2_OP_DIFF:   01 RRGGBB = Diff from previous pixel, -2 to 1
   QOI2_OP_LUMA:   10 GGGGGG RRRR BBBB = Diff from previous pixel, G -32 to 31, R and B diff to green diff -8 to 7
   QOI2_OP_RUN:    110 YYYYY = Repeat previous pixel Y+1 times, up to 32.
   QOI2_OP_RUN_EX: 111 YYYYY = Repeat previous pixel (Y+2)*32 times, up to YYYYY=11011
   QOI2_OP_RGB_EX: 111111 XX RRRRRRRR GGGGGGGG BBBBBBBB = X+1 literal RGB values
   QOI2_OP_RGBA:   01101010  RRRRRRRR GGGGGGGG BBBBBBBB AAAAAAAA = Literal RGBA value

*/


/* -----------------------------------------------------------------------------
Header - Public functions */

#ifndef QOI2_H
#define QOI2_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A pointer to a qoi_desc struct has to be supplied to all of qoi's functions.
It describes either the input format (for qoi2_write and qoi2_encode), or is
filled with the description read from the file header (for qoi2_read and
qoi2_decode).

The colorspace in this qoi2_desc is an enum where
	0 = sRGB, i.e. gamma scaled RGB channels and a linear alpha channel
	1 = all channels are linear
You may use the constants QOI2_SRGB or QOI2_LINEAR. The colorspace is purely
informative. It will be saved to the file header, but does not affect
how chunks are en-/decoded. */

#define QOI2_SRGB   0
#define QOI2_LINEAR 1

#define QOI2_BARE_HEADER_SIZE 14

#ifndef QOI2_ALLOW_HEAP
	#define QOI2_ALLOW_HEAP 1
#endif

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned char channels;
	unsigned char colorspace;
} qoi2_desc;

#if QOI2_ALLOW_HEAP
// Single-step API with heap memory allocation
int qoi2_encode(const unsigned char *pixels, int input_channels, const qoi2_desc *desc, unsigned char **bytes, size_t *byte_count);
int qoi2_decode(const unsigned char *bytes, size_t byte_count, qoi2_desc *desc, unsigned char **pixels, int output_channels);
#endif

// Multi-step API without heap memory allocation
size_t qoi2_max_encoded_data_size_without_header(const qoi2_desc *desc);
int qoi2_encode_header(const qoi2_desc *desc, unsigned char *bytes);
int qoi2_decode_header(const unsigned char *bytes, size_t byte_count, qoi2_desc *desc);
int qoi2_encode_data(const unsigned char *pixels, int input_channels, const qoi2_desc *desc, const unsigned char *reference_pixels, unsigned char *bytes, size_t *byte_count);
int qoi2_decode_data(const unsigned char *bytes, size_t byte_count, const qoi2_desc *desc, const unsigned char *reference_pixels, unsigned char *pixels, int output_channels);

#ifdef __cplusplus
}
#endif
#endif /* QOI2_H */


/* -----------------------------------------------------------------------------
Implementation */

#ifdef QOI2_IMPLEMENTATION

#define QOI2_OP_INDEX  0x00 /* 00xxxxxx */
#define QOI2_OP_DIFF   0x40 /* 01xxxxxx */
#define QOI2_OP_RGBA   0x6a /* 01101010 */
#define QOI2_OP_LUMA   0x80 /* 10xxxxxx */
#define QOI2_OP_RUN    0xc0 /* 110xxxxx */
#define QOI2_OP_RUN_EX 0xe0 /* 111xxxxx */
#define QOI2_OP_RGB_EX 0xfc /* 111111xx */

#define QOI2_MASK_2    0xc0 /* 11000000 */
#define QOI2_MASK_3    0xe0 /* 11100000 */

#define QOI2_COLOR_HASH(C) (C.rgba.r*3 + C.rgba.g*5 + C.rgba.b*7 + C.rgba.a*11)
#define QOI2_MAGIC \
	(((unsigned int)'q')       | ((unsigned int)'o') <<  8 | \
	 ((unsigned int)'i') << 16 | ((unsigned int)'2') << 24 )

typedef union {
	struct { unsigned char r, g, b, a; } rgba;
	unsigned int v;
} qoi2_rgba_t;

static void qoi2_write_32(unsigned char *bytes, size_t *p, unsigned int v) {
	bytes[(*p)++] = (0x000000ff & v);
	bytes[(*p)++] = (0x0000ff00 & v) >> 8;
	bytes[(*p)++] = (0x00ff0000 & v) >> 16;
	bytes[(*p)++] = (0xff000000 & v) >> 24;
}

static unsigned int qoi2_read_32(const unsigned char *bytes, size_t *p) {
	unsigned int a = bytes[(*p)++];
	unsigned int b = bytes[(*p)++];
	unsigned int c = bytes[(*p)++];
	unsigned int d = bytes[(*p)++];
	return d << 24 | c << 16 | b << 8 | a;
}

#if QOI2_ALLOW_HEAP

#ifndef QOI_MALLOC
	#include <stdlib.h>
	#define QOI_MALLOC(sz) malloc(sz)
	#define QOI_FREE(p)    free(p)
#endif

int qoi2_encode(const unsigned char *pixels, int input_channels, const qoi2_desc *desc, unsigned char **bytes, size_t *byte_count) {
	size_t header_size, max_data_size, required_buffer_size, written_data_byte_count;
	unsigned char *temp_bytes = NULL;
	*bytes = NULL;
	*byte_count = 0;

	if (pixels == NULL || desc == NULL || bytes == NULL || byte_count == NULL) {
		return 0;
	}

	header_size = QOI2_BARE_HEADER_SIZE;
	max_data_size = qoi2_max_encoded_data_size_without_header(desc);
	required_buffer_size = header_size + max_data_size;

	temp_bytes = (unsigned char*)QOI_MALLOC(required_buffer_size);
	if (!temp_bytes) {
		return 0;
	}

	if (!qoi2_encode_header(desc, temp_bytes)) {
		QOI_FREE(temp_bytes);
		return 0;
	}

	written_data_byte_count = max_data_size;
	if (!qoi2_encode_data(pixels, input_channels, desc, NULL, temp_bytes + header_size, &written_data_byte_count)) {
		QOI_FREE(temp_bytes);
		return 0;
	}

	*bytes = temp_bytes;
	*byte_count = header_size + written_data_byte_count;
	return 1;
}

int qoi2_decode(const unsigned char *bytes, size_t byte_count, qoi2_desc *desc, unsigned char **pixels, int output_channels) {
	size_t header_size;
	size_t px_len;
	unsigned char *temp_pixels = NULL;

	if (bytes == NULL || desc == NULL || pixels == NULL) {
		return 0;
	}

	if (!qoi2_decode_header(bytes, byte_count, desc)) {
		return 0;
	}
	header_size = QOI2_BARE_HEADER_SIZE;
	output_channels = (output_channels == 0 ? desc->channels : output_channels);

	px_len = desc->width * desc->height * output_channels;
	temp_pixels = (unsigned char *)QOI_MALLOC(px_len);
	if (!temp_pixels) {
		return 0;
	}

	if (!qoi2_decode_data(bytes + header_size, byte_count - header_size, desc, NULL, temp_pixels, output_channels)) {
		QOI_FREE(temp_pixels);
		return 0;
	}

	*pixels = temp_pixels;
	return 1;
}

#endif /* QOI2_ALLOW_HEAP */

size_t qoi2_max_encoded_data_size_without_header(const qoi2_desc *desc) {
	size_t pixel_count = desc->width * desc->height;
	size_t uncompressed_size = pixel_count * desc->channels;
	size_t max_compressed_size;
	if (desc->channels < 4) {
		// Worst case for RGB is every pixel in an RGB run of 4 pixels, plus one RGB value at the end.
		max_compressed_size = uncompressed_size + ((pixel_count / 4) + 1);
	}
	else {
		// Worst case for RGBA is every pixel as an RGBA tag
		max_compressed_size = uncompressed_size + pixel_count;
	}
	return max_compressed_size;
}

int qoi2_encode_header(const qoi2_desc *desc, unsigned char *bytes) {
	size_t p = 0;

	if (
		bytes == NULL || desc == NULL ||
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc->channels > 4 ||
		desc->colorspace > 1
	) {
		return 0;
	}

	qoi2_write_32(bytes, &p, QOI2_MAGIC);
	qoi2_write_32(bytes, &p, desc->width);
	qoi2_write_32(bytes, &p, desc->height);
	bytes[p++] = desc->channels;
	bytes[p++] = desc->colorspace;

	return 1;
}

int qoi2_decode_header(const unsigned char *bytes, size_t byte_count, qoi2_desc *desc) {
	size_t p = 0;
	unsigned int header_magic;

	if (bytes == NULL || desc == NULL || (byte_count < QOI2_BARE_HEADER_SIZE)) {
		return 0;
	}

	header_magic = qoi2_read_32(bytes, &p);
	desc->width = qoi2_read_32(bytes, &p);
	desc->height = qoi2_read_32(bytes, &p);
	desc->channels = bytes[p++];
	desc->colorspace = bytes[p++];

	if (
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc->channels > 4 ||
		desc->colorspace > 1 ||
		header_magic != QOI2_MAGIC
	) {
		return 0;
	}

	return 1;
}

int qoi2_encode_data(const unsigned char *pixels, int input_channels, const qoi2_desc *desc, const unsigned char *reference_pixels, unsigned char *bytes, size_t *byte_count) {
	size_t p;
	size_t px_len, px_end, px_pos;
	int run;
	qoi2_rgba_t px, px_prev;
	qoi2_rgba_t index[64] = { 0 };
	int in_literal_rgb_run = 0;
	int include_alpha;
	int literal_rgb_run_length = 0;
	size_t literal_rgb_marker_pos = 0;
	size_t max_buffer_size;

	if (
		pixels == NULL || bytes == NULL || byte_count == NULL || desc == NULL ||
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc->channels > 4 ||
		input_channels < 3 || input_channels > 4 ||
		desc->colorspace > 1
	) {
		return 0;
	}

	// Ensure that the output buffer is large enough to hold our worst-case output data
	max_buffer_size = qoi2_max_encoded_data_size_without_header(desc);
	if (max_buffer_size > *byte_count) {
		return 0;
	}

	p = 0;
	run = 0;
	px_prev.rgba.r = 0;
	px_prev.rgba.g = 0;
	px_prev.rgba.b = 0;
	px_prev.rgba.a = 255;
	px = px_prev;

	px_len = desc->width * desc->height * desc->channels;
	px_end = px_len - desc->channels;
	include_alpha = (desc->channels == 4) && (input_channels == 4);

	for (px_pos = 0; px_pos < px_len; px_pos += input_channels) {
		int same_value_as_previous;

		px.rgba.r = pixels[px_pos + 0];
		px.rgba.g = pixels[px_pos + 1];
		px.rgba.b = pixels[px_pos + 2];
		if (include_alpha) {
			px.rgba.a = pixels[px_pos + 3];
		}

		if (reference_pixels != NULL) {
			px.rgba.r -= reference_pixels[px_pos + 0];
			px.rgba.g -= reference_pixels[px_pos + 1];
			px.rgba.b -= reference_pixels[px_pos + 2];
			if (include_alpha) {
				px.rgba.a -= reference_pixels[px_pos + 3];
			}
		}

		same_value_as_previous = (px.v == px_prev.v);
		if (same_value_as_previous) {
			++run;
		}

		if ((run > 0) && (!same_value_as_previous || (px_pos == px_end))) {
			int longRunCount = run >> 5;
			while (longRunCount >= 29) {
				bytes[p++] = QOI2_OP_RUN_EX | 0b11011;
				longRunCount -= 29;
			}
			if (longRunCount >= 2) {
				bytes[p++] = QOI2_OP_RUN_EX | (longRunCount - 2);
			}
			else if (longRunCount == 1) {
				bytes[p++] = QOI2_OP_RUN | (32 - 1);
			}
			run &= 0b11111;
			if (run > 0) {
				bytes[p++] = QOI2_OP_RUN | (run - 1);
			}
			run = 0;
			in_literal_rgb_run = 0;
		}

		if (!same_value_as_previous) {
			int index_pos = QOI2_COLOR_HASH(px) & (64 - 1);

			if (index[index_pos].v == px.v) {
				bytes[p++] = QOI2_OP_INDEX | index_pos;
				in_literal_rgb_run = 0;
			}
			else {
				index[index_pos] = px;

				if (px.rgba.a == px_prev.rgba.a) {
					signed char vr = px.rgba.r - px_prev.rgba.r;
					signed char vg = px.rgba.g - px_prev.rgba.g;
					signed char vb = px.rgba.b - px_prev.rgba.b;

					signed char vg_r = vr - vg;
					signed char vg_b = vb - vg;

					if (
						vr > -3 && vr < 2 &&
						vg > -3 && vg < 2 &&
						vb > -3 && vb < 2
					) {
						bytes[p++] = QOI2_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2);
						in_literal_rgb_run = 0;
					}
					else if (
						vg_r >  -9 && vg_r <  8 &&
						vg   > -33 && vg   < 32 &&
						vg_b >  -9 && vg_b <  8
					) {
						bytes[p++] = QOI2_OP_LUMA     | (vg   + 32);
						bytes[p++] = (vg_r + 8) << 4 | (vg_b +  8);
						in_literal_rgb_run = 0;
					}
					else {
						if (in_literal_rgb_run) {
							++literal_rgb_run_length;
							if (literal_rgb_run_length < 4) {
								bytes[literal_rgb_marker_pos] = QOI2_OP_RGB_EX | literal_rgb_run_length;
							}
							else {
								in_literal_rgb_run = 0;
							}
						}
						if (!in_literal_rgb_run) {
							in_literal_rgb_run = 1;
							literal_rgb_marker_pos = p;
							literal_rgb_run_length = 0;
							bytes[p++] = QOI2_OP_RGB_EX;
						}
						bytes[p++] = px.rgba.r;
						bytes[p++] = px.rgba.g;
						bytes[p++] = px.rgba.b;
					}
				}
				else {
					bytes[p++] = QOI2_OP_RGBA;
					bytes[p++] = px.rgba.r;
					bytes[p++] = px.rgba.g;
					bytes[p++] = px.rgba.b;
					bytes[p++] = px.rgba.a;
					in_literal_rgb_run = 0;
				}
			}
		}
		px_prev = px;
	}

	*byte_count = p;
	return 1;
}

int qoi2_decode_data(const unsigned char *bytes, size_t byte_count, const qoi2_desc *desc, const unsigned char *reference_pixels, unsigned char *pixels, int output_channels) {
	qoi2_rgba_t index[64] = { 0 };
	qoi2_rgba_t px;
	size_t px_len, px_pos;
	int run = 0;
	int rgb_run = 0;
	size_t p = 0;

	output_channels = (output_channels == 0 ? desc->channels : output_channels);

	if (
		bytes == NULL || desc == NULL ||
		(output_channels != 3 && output_channels != 4)
	) {
		return 0;
	}

	if (
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc->channels > 4 ||
		desc->colorspace > 1
	) {
		return 0;
	}

	px_len = desc->width * desc->height * output_channels;

	px.rgba.r = 0;
	px.rgba.g = 0;
	px.rgba.b = 0;
	px.rgba.a = 255;

	for (px_pos = 0; px_pos < px_len; px_pos += output_channels) {
		if (run > 0) {
			run--;
			if (rgb_run) {
				px.rgba.r = bytes[p++];
				px.rgba.g = bytes[p++];
				px.rgba.b = bytes[p++];
				index[QOI2_COLOR_HASH(px) & (64 - 1)] = px;
			}
		}
		else {
			unsigned char b1;
			if (p >= byte_count) {
				return 0;
			}
			b1 = bytes[p++];
			rgb_run = 0;

			if (b1 == QOI2_OP_RGBA) {
				if (p + 3 >= byte_count) {
					return 0;
				}
				px.rgba.r = bytes[p++];
				px.rgba.g = bytes[p++];
				px.rgba.b = bytes[p++];
				px.rgba.a = bytes[p++];
			}
			else if ((b1 & QOI2_OP_RGB_EX) == QOI2_OP_RGB_EX) {
				if (p + 2 >= byte_count) {
					return 0;
				}
				px.rgba.r = bytes[p++];
				px.rgba.g = bytes[p++];
				px.rgba.b = bytes[p++];
				run = (b1 & 0x03);
				rgb_run = 1;
			}
			else if ((b1 & QOI2_MASK_2) == QOI2_OP_INDEX) {
				px = index[b1];
			}
			else if ((b1 & QOI2_MASK_2) == QOI2_OP_DIFF) {
				px.rgba.r += ((b1 >> 4) & 0x03) - 2;
				px.rgba.g += ((b1 >> 2) & 0x03) - 2;
				px.rgba.b += ( b1       & 0x03) - 2;
			}
			else if ((b1 & QOI2_MASK_2) == QOI2_OP_LUMA) {
				if (p >= byte_count) {
					return 0;
				}
				int b2 = bytes[p++];
				int vg = (b1 & 0x3f) - 32;
				px.rgba.r += vg - 8 + ((b2 >> 4) & 0x0f);
				px.rgba.g += vg;
				px.rgba.b += vg - 8 +  (b2       & 0x0f);
			}
			else if ((b1 & QOI2_MASK_3) == QOI2_OP_RUN) {
				run = (b1 & 0x1f);
			}
			else if ((b1 & QOI2_MASK_3) == QOI2_OP_RUN_EX) {
				run = (((b1 & 0x1f) + 2) * 32) - 1;
			}

			index[QOI2_COLOR_HASH(px) & (64 - 1)] = px;
		}

		if (reference_pixels == NULL) {
			pixels[px_pos + 0] = px.rgba.r;
			pixels[px_pos + 1] = px.rgba.g;
			pixels[px_pos + 2] = px.rgba.b;

			if (output_channels == 4) {
				pixels[px_pos + 3] = px.rgba.a;
			}
		}
		else {
			pixels[px_pos + 0] = px.rgba.r + reference_pixels[px_pos + 0];
			pixels[px_pos + 1] = px.rgba.g + reference_pixels[px_pos + 1];
			pixels[px_pos + 2] = px.rgba.b + reference_pixels[px_pos + 2];
			if (output_channels == 4) {
				pixels[px_pos + 3] = px.rgba.a + reference_pixels[px_pos + 3];
			}
		}
	}

	return 1;
}

#endif /* QOI2_IMPLEMENTATION */
