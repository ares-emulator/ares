sljit.objects := sljitLir sljitAllocator
sljit.objects := $(sljit.objects:%=$(object.path)/%.o)

$(object.path)/sljitLir.o: $(sljit.path)/sljitLir.c
$(object.path)/sljitAllocator.o: $(thirdparty.path)/sljitAllocator.cpp

flags += -DSLJIT_HAVE_CONFIG_PRE=1 -DSLJIT_HAVE_CONFIG_POST=1
