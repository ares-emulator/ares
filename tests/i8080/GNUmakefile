name := i8080
build := optimized
threaded := true
openmp := false
vulkan := true
local := true
lto := true
console := true
flags += -I. -I../.. -I../../ares -I../../thirdparty -DMIA_LIBRARY

nall.path := ../../nall
include $(nall.path)/GNUmakefile

ifneq ($(filter $(arch),x86 amd64),)
  ifeq ($(filter cl,$(compiler)),)
    ifeq ($(local),true)
      flags += -march=native
    else
      # For official builds, default to x86-64-v2 (Intel Nehalem, AMD Bulldozer) which supports up to SSE 4.2.
      flags += -march=x86-64-v2
    endif
  endif
endif

thirdparty.path := ../../thirdparty
libchdr.path := $(thirdparty.path)/libchdr
include $(thirdparty.path)/GNUmakefile

profile := performance
ares.components := i8080

ares.path := ../../ares
include $(ares.path)/GNUmakefile

i8080.path = ../i8080

i8080.objects += i8080
i8080.objects := $(i8080.objects:%=$(object.path)/%.o)

$(object.path)/i8080.o: $(i8080.path)/i8080.cpp

all.objects := $(nall.objects) $(libchdr.objects) $(ares.objects) $(i8080.objects)
all.options := $(nall.options) $(libchdr.options) $(ares.options) $(i8080.options) $(options)

$(all.objects): | $(object.path)

all: $(all.objects) | $(output.path)
	$(info Linking $(output.path)/$(name)$(extension) ...)
	+@$(compiler) $(call exe,$(output.path)/$(name)$(extension)) $(all.objects) $(all.options)

verbose: nall.verbose ruby.verbose hiro.verbose all;

clean:
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
endif
	$(call rdelete,$(object.path))
	$(call rdelete,$(output.path))

install: all
ifeq ($(platform),windows)
	$(call mkdir,$(prefix)/$(name)/)
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	mkdir -p ~/Library/Application\ Support/$(name)/
	cp -R $(output.path)/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/hicolor/256x256/apps/
	mkdir -p $(prefix)/share/$(name)/Shaders/
	mkdir -p $(prefix)/share/$(name)/Database/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
	cp -R $(ares.path)/Shaders/* $(prefix)/share/$(name)/Shaders/
	cp -R $(mia.path)/Database/* $(prefix)/share/$(name)/Database/
	cp resource/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp resource/$(name).png $(prefix)/share/icons/hicolor/256x256/apps/$(name).png
endif

uninstall:
ifeq ($(platform),windows)
else ifeq ($(shell id -un),root)
	$(error "make uninstall should not be run as root")
else ifeq ($(platform),macos)
	rm -rf /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	rm -f $(prefix)/bin/$(name)
	rm -f $(prefix)/share/applications/$(name).desktop
	rm -f $(prefix)/share/icons/hicolor/256x256/apps/$(name).png
endif

-include $(object.path)/*.d
