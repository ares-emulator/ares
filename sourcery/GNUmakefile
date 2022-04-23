name := sourcery
build := stable
flags += -I..

nall.path := ../nall
include $(nall.path)/GNUmakefile

objects := obj/sourcery.o

obj/sourcery.o: sourcery.cpp

all: $(objects)
	$(info Linking out/$(name) ...)
	+@$(compiler) -o out/$(name) $(objects) $(options)

verbose: nall.verbose all;

clean:
	$(call delete,obj/*)
	$(call delete,out/*)

install: all
ifneq ($(filter $(platform),linux bsd),)
	cp out/$(name) $(prefix)/bin/$(name)
endif

uninstall:
ifneq ($(filter $(platform),linux bsd),)
	rm -f $(prefix)/bin/$(name)
endif
