openmp := false
local := true
ifeq ($(platform),local)
  flags += -march=native
endif

nall.path := ../nall
include $(nall.path)/GNUmakefile

libco.path := ../libco
include $(libco.path)/GNUmakefile

ruby.path := ../ruby
include $(ruby.path)/GNUmakefile

hiro.path := ../hiro
hiro.resource := resource/lucia.rc
include $(hiro.path)/GNUmakefile

profile := performance
cores := fc sfc sg ms md pce msx gb gba ws ngp

ares.path := ../ares
include $(ares.path)/GNUmakefile

name := lucia
flags += -I. -I.. -I../ares -DMIA_LIBRARY

mia.objects := mia mia-resource
mia.objects := $(mia.objects:%=$(object.path)/%.o)

$(object.path)/mia.o: ../mia/mia.cpp
$(object.path)/mia-resource.o: ../mia/resource/resource.cpp

lucia.objects += lucia lucia-resource lucia-emulators lucia-program lucia-input
lucia.objects += lucia-presentation lucia-settings lucia-tools
lucia.objects := $(lucia.objects:%=$(object.path)/%.o)

$(object.path)/lucia.o: lucia.cpp
$(object.path)/lucia-resource.o: resource/resource.cpp
$(object.path)/lucia-emulators.o: emulators/emulators.cpp
$(object.path)/lucia-program.o: program/program.cpp
$(object.path)/lucia-input.o: input/input.cpp
$(object.path)/lucia-presentation.o: presentation/presentation.cpp
$(object.path)/lucia-settings.o: settings/settings.cpp
$(object.path)/lucia-tools.o: tools/tools.cpp

all.objects := $(libco.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(mia.objects) $(lucia.objects)
all.options := $(libco.options) $(ruby.options) $(hiro.options) $(ares.options) $(mia.options) $(lucia.options) $(options)

all: $(all.objects)
	$(info Linking out/$(name) ...)
	+@$(compiler) -o out/$(name) $(all.objects) $(all.options)
ifeq ($(platform),macos)
	rm -rf out/$(name).app
	mkdir -p out/$(name).app/Contents/MacOS/
	mkdir -p out/$(name).app/Contents/Resources/
	mv out/$(name) out/$(name).app/Contents/MacOS/$(name)
	cp resource/$(name).plist out/$(name).app/Contents/Info.plist
	sips -s format icns resource/$(name).png --out out/$(name).app/Contents/Resources/$(name).icns
endif

verbose: nall.verbose ruby.verbose hiro.verbose all;

install: all
ifeq ($(platform),windows)
	mkdir -p $(prefix)/$(name)/
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	mkdir -p ~/Library/Application\ Support/$(name)/
	cp -R out/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	cp out/$(name) $(prefix)/bin/$(name)
	cp resource/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp resource/$(name).png $(prefix)/share/icons/$(name).png
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
	rm -f $(prefix)/share/icons/$(name).png
endif

clean:
	$(call delete,obj/*)
	$(call delete,out/*)

-include obj/*.d
