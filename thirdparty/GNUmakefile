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

tzxfile.objects := TZXAudioGenerator TZXBlock TZXBlockArchiveInfo TZXBlockCustomInfo
tzxfile.objects += TZXBlockGroupEnd TZXBlockGroupStart TZXBlockHardwareType TZXBlockLoopEnd
tzxfile.objects += TZXBlockLoopStart TZXBlockMessage TZXBlockPause TZXBlockPulseSequence
tzxfile.objects += TZXBlockPureData TZXBlockPureTone TZXBlockStandardSpeedData
tzxfile.objects += TZXBlockStopTheTape48K TZXBlockTextDescription TZXBlockTurboSpeedData TZXFile
tzxfile.objects := $(tzxfile.objects:%=$(object.path)/%.o)

$(object.path)/TZXAudioGenerator.o: $(tzxfile.path)/TZXAudioGenerator.cpp
$(object.path)/TZXBlock.o: $(tzxfile.path)/TZXBlock.cpp
$(object.path)/TZXBlockArchiveInfo.o: $(tzxfile.path)/TZXBlockArchiveInfo.cpp
$(object.path)/TZXBlockCustomInfo.o: $(tzxfile.path)/TZXBlockCustomInfo.cpp
$(object.path)/TZXBlockGroupEnd.o: $(tzxfile.path)/TZXBlockGroupEnd.cpp
$(object.path)/TZXBlockGroupStart.o: $(tzxfile.path)/TZXBlockGroupStart.cpp
$(object.path)/TZXBlockHardwareType.o: $(tzxfile.path)/TZXBlockHardwareType.cpp
$(object.path)/TZXBlockLoopEnd.o: $(tzxfile.path)/TZXBlockLoopEnd.cpp
$(object.path)/TZXBlockLoopStart.o: $(tzxfile.path)/TZXBlockLoopStart.cpp
$(object.path)/TZXBlockMessage.o: $(tzxfile.path)/TZXBlockMessage.cpp
$(object.path)/TZXBlockPause.o: $(tzxfile.path)/TZXBlockPause.cpp
$(object.path)/TZXBlockPulseSequence.o: $(tzxfile.path)/TZXBlockPulseSequence.cpp
$(object.path)/TZXBlockPureData.o: $(tzxfile.path)/TZXBlockPureData.cpp
$(object.path)/TZXBlockPureTone.o: $(tzxfile.path)/TZXBlockPureTone.cpp
$(object.path)/TZXBlockStandardSpeedData.o: $(tzxfile.path)/TZXBlockStandardSpeedData.cpp
$(object.path)/TZXBlockStopTheTape48K.o: $(tzxfile.path)/TZXBlockStopTheTape48K.cpp
$(object.path)/TZXBlockTextDescription.o: $(tzxfile.path)/TZXBlockTextDescription.cpp
$(object.path)/TZXBlockTurboSpeedData.o: $(tzxfile.path)/TZXBlockTurboSpeedData.cpp
$(object.path)/TZXFile.o: $(tzxfile.path)/TZXFile.cpp

flags += -I$(tzxfile.path)