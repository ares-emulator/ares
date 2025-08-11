/*

Copyright (c) 2025, Roger Sanders

SPDX-License-Identifier: MIT

*/

#ifndef QON_H
#define QON_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QON_SRGB   0
#define QON_LINEAR 1

#define QON_FLAGS_USES_INTERFRAME_COMPRESSION  0x0001
#define QON_FLAGS_LOOP_ANIMATION               0x0002
#define QON_FRAME_FLAGS_INTERFRAME_COMPRESSION 0x8000

#define QON_BARE_HEADER_SIZE 24
#define QON_INDEX_SIZE_PER_ENTRY 8
#define QON_FRAME_SIZE_SIZE 4

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned char channels;
	unsigned char colorspace;
	unsigned short flags;
	unsigned int frame_count;
	unsigned int frame_duration_in_microseconds;
} qon_desc;

int qon_encode_header(const qon_desc *desc, unsigned char *bytes);
int qon_decode_header(const unsigned char *bytes, size_t byte_count, qon_desc *desc);
void qon_encode_index_entry(unsigned char *index_base, size_t index, size_t offset, unsigned short frame_flags);
void qon_decode_index_entry(const unsigned char *index_base, size_t index, size_t *offset, unsigned short *frame_flags);
void qon_encode_frame_size(unsigned char *frame_base, size_t frame_size);
size_t qon_decode_frame_size(const unsigned char *frame_base);

#ifdef __cplusplus
}
#endif
#endif /* QON_H */


/* -----------------------------------------------------------------------------
Implementation */

#ifdef QON_IMPLEMENTATION

#define QON_MAGIC \
	(((unsigned int)'q')       | ((unsigned int)'o') <<  8 | \
	 ((unsigned int)'n') << 16 | ((unsigned int)'1') << 24 )

static void qon_write_64(unsigned char *bytes, unsigned long long v) {
	*(bytes++) = (0x00000000000000ffull & v);
	*(bytes++) = (0x000000000000ff00ull & v) >> 8;
	*(bytes++) = (0x0000000000ff0000ull & v) >> 16;
	*(bytes++) = (0x00000000ff000000ull & v) >> 24;
	*(bytes++) = (0x000000ff00000000ull & v) >> 32;
	*(bytes++) = (0x0000ff0000000000ull & v) >> 40;
	*(bytes++) = (0x00ff000000000000ull & v) >> 48;
	*(bytes++) = (0xff00000000000000ull & v) >> 56;
}

static unsigned long long qon_read_64(const unsigned char *bytes) {
	unsigned long long a = *(bytes++);
	unsigned long long b = *(bytes++);
	unsigned long long c = *(bytes++);
	unsigned long long d = *(bytes++);
	unsigned long long e = *(bytes++);
	unsigned long long f = *(bytes++);
	unsigned long long g = *(bytes++);
	unsigned long long h = *(bytes++);
	return h << 56 | g << 48 | f << 40 | e << 32 | d << 24 | c << 16 | b << 8 | a;
}

static void qon_write_32(unsigned char *bytes, unsigned int v) {
	*(bytes++) = (0x000000ff & v);
	*(bytes++) = (0x0000ff00 & v) >> 8;
	*(bytes++) = (0x00ff0000 & v) >> 16;
	*(bytes++) = (0xff000000 & v) >> 24;
}

static unsigned int qon_read_32(const unsigned char *bytes) {
	unsigned int a = *(bytes++);
	unsigned int b = *(bytes++);
	unsigned int c = *(bytes++);
	unsigned int d = *(bytes++);
	return d << 24 | c << 16 | b << 8 | a;
}

static void qon_write_16(unsigned char *bytes, unsigned short v) {
	*(bytes++) = (0x000000ff & v);
	*(bytes++) = (0x0000ff00 & v) >> 8;
}

static unsigned short qon_read_16(const unsigned char *bytes) {
	unsigned int a = *(bytes++);
	unsigned int b = *(bytes++);
	return b << 8 | a;
}

int qon_encode_header(const qon_desc *desc, unsigned char *bytes) {
	if (
		bytes == NULL || desc == NULL ||
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc->channels > 4 ||
		desc->colorspace > 1 ||
		desc->frame_count == 0
	) {
		return 0;
	}

	qon_write_32(bytes, QON_MAGIC);
	bytes += 4;
	qon_write_32(bytes, desc->width);
	bytes += 4;
	qon_write_32(bytes, desc->height);
	bytes += 4;
	*(bytes++) = desc->channels;
	*(bytes++) = desc->colorspace;
	qon_write_16(bytes, desc->flags);
	bytes += 2;
	qon_write_32(bytes, desc->frame_count);
	bytes += 4;
	qon_write_32(bytes, desc->frame_duration_in_microseconds);
	bytes += 4;

	return 1;
}

int qon_decode_header(const unsigned char *bytes, size_t byte_count, qon_desc *desc) {
	size_t p = 0;
	unsigned int header_magic;

	if (bytes == NULL || desc == NULL || (byte_count < QON_BARE_HEADER_SIZE)) {
		return 0;
	}

	header_magic = qon_read_32(bytes);
	bytes += 4;
	desc->width = qon_read_32(bytes);
	bytes += 4;
	desc->height = qon_read_32(bytes);
	bytes += 4;
	desc->channels = *(bytes++);
	desc->colorspace = *(bytes++);
	desc->flags = qon_read_16(bytes);
	bytes += 2;
	desc->frame_count = qon_read_32(bytes);
	bytes += 4;
	desc->frame_duration_in_microseconds = qon_read_32(bytes);

	if (
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc->channels > 4 ||
		desc->colorspace > 1 ||
		desc->frame_count == 0 ||
		header_magic != QON_MAGIC
	) {
		return 0;
	}

	return 1;
}

void qon_encode_index_entry(unsigned char *index_base, size_t index, size_t offset, unsigned short frame_flags) {
	size_t index_entry_pos = (index << 3);
	unsigned long long index_entry = (unsigned long long)offset | ((unsigned long long)frame_flags << 48);
	qon_write_64(index_base + index_entry_pos, index_entry);
}

void qon_decode_index_entry(const unsigned char *index_base, size_t index, size_t *offset, unsigned short *frame_flags) {
	size_t index_entry_pos = (index << 3);
	unsigned long long index_entry = qon_read_64(index_base + index_entry_pos);
	*offset = index_entry & 0x0000FFFFFFFFFFFFull;
	*frame_flags = index_entry >> 48;
}

void qon_encode_frame_size(unsigned char *frame_base, size_t frame_size) {
	qon_write_32(frame_base, (int)frame_size);
}

size_t qon_decode_frame_size(const unsigned char *frame_base) {
	return qon_read_32(frame_base);
}

#endif /* QON_IMPLEMENTATION */
