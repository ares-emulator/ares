sljit.objects := sljitLir sljitAllocator
sljit.objects := $(sljit.objects:%=$(object.path)/%.o)

$(object.path)/sljitLir.o: $(sljit.path)/sljitLir.c
$(object.path)/sljitAllocator.o: $(thirdparty.path)/sljitAllocator.cpp

flags += -DSLJIT_HAVE_CONFIG_PRE=1 -DSLJIT_HAVE_CONFIG_POST=1

libchdr.objects := libchdr_bitstream libchdr_cdrom libchdr_chd libchdr_flac libchdr_huffman
libchdr.objects += lzma_alloc lzma_bra86 lzma_braia64 lzma_cpuarch lzma_delta lzma_lfind
libchdr.objects += lzma_86dec lzma_dec lcma_enc lzma_sort
libchdr.objects += zlib_adler32 zlib_compress zlib_crc32 zlib_deflate zlib_infback zlib_inffast
libchdr.objects += zlib_inflate zlib_inftrees zlib_trees zlib_uncompr zlib_zutil

libchdr.objects := $(libchdr.objects:%=$(object.path)/%.o)

$(object.path)/libchdr_bitstream.o: $(libchdr.path)/src/libchdr_bitstream.c
$(object.path)/libchdr_cdrom.o: $(libchdr.path)/src/libchdr_cdrom.c
$(object.path)/libchdr_chd.o: $(libchdr.path)/src/libchdr_chd.c
$(object.path)/libchdr_flac.o: $(libchdr.path)/src/libchdr_flac.c
$(object.path)/libchdr_huffman.o: $(libchdr.path)/src/libchdr_huffman.c

$(object.path)/lzma_alloc.o: $(libchdr.path)/deps/lzma-19.00/src/Alloc.c
$(object.path)/lzma_bra86.o: $(libchdr.path)/deps/lzma-19.00/src/Bra86.c
$(object.path)/lzma_braia64.o: $(libchdr.path)/deps/lzma-19.00/src/BraIA64.c
$(object.path)/lzma_cpuarch.o: $(libchdr.path)/deps/lzma-19.00/src/CpuArch.c
$(object.path)/lzma_delta.o: $(libchdr.path)/deps/lzma-19.00/src/Delta.c
$(object.path)/lzma_lfind.o: $(libchdr.path)/deps/lzma-19.00/src/LzFind.c
$(object.path)/lzma_86dec.o: $(libchdr.path)/deps/lzma-19.00/src/Lzma86Dec.c
$(object.path)/lzma_dec.o: $(libchdr.path)/deps/lzma-19.00/src/LzmaDec.c
$(object.path)/lcma_enc.o: $(libchdr.path)/deps/lzma-19.00/src/LzmaEnc.c
$(object.path)/lzma_sort.o: $(libchdr.path)/deps/lzma-19.00/src/Sort.c

$(object.path)/zlib_adler32.o: $(libchdr.path)/deps/zlib-1.2.13/adler32.c
$(object.path)/zlib_compress.o: $(libchdr.path)/deps/zlib-1.2.13/compress.c
$(object.path)/zlib_crc32.o: $(libchdr.path)/deps/zlib-1.2.13/crc32.c
$(object.path)/zlib_deflate.o: $(libchdr.path)/deps/zlib-1.2.13/deflate.c
$(object.path)/zlib_infback.o: $(libchdr.path)/deps/zlib-1.2.13/infback.c
$(object.path)/zlib_inffast.o: $(libchdr.path)/deps/zlib-1.2.13/inffast.c
$(object.path)/zlib_inflate.o: $(libchdr.path)/deps/zlib-1.2.13/inflate.c
$(object.path)/zlib_inftrees.o: $(libchdr.path)/deps/zlib-1.2.13/inftrees.c
$(object.path)/zlib_trees.o: $(libchdr.path)/deps/zlib-1.2.13/trees.c
$(object.path)/zlib_uncompr.o: $(libchdr.path)/deps/zlib-1.2.13/uncompr.c
$(object.path)/zlib_zutil.o: $(libchdr.path)/deps/zlib-1.2.13/zutil.c

flags += -I$(libchdr.path)/include
flags += -I$(libchdr.path)/deps/lzma-19.00/include -D_7ZIP_ST
flags += -I$(libchdr.path)/deps/zlib-1.2.13

# instruct glibc to declare fseeko/ftello
flags += -D_LARGEFILE_SOURCE
