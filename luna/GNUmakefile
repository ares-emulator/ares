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
hiro.resource := $(ui)/resource/luna.rc
include $(hiro.path)/GNUmakefile

profile := accuracy
cores := fc sfc sg ms md pce msx cv gb gba ws ngp

ares.path := ../ares
include $(ares.path)/GNUmakefile

name := luna
flags += -I. -I.. -I../ares

luna.objects += luna luna-resource luna-emulator
luna.objects += luna-input luna-menus luna-program luna-settings
luna.objects := $(luna.objects:%=$(object.path)/%.o)

$(object.path)/luna.o: luna.cpp
$(object.path)/luna-resource.o: resource/resource.cpp
$(object.path)/luna-emulator.o: emulator/emulator.cpp
$(object.path)/luna-input.o: input/input.cpp
$(object.path)/luna-menus.o: menus/menus.cpp
$(object.path)/luna-program.o: program/program.cpp
$(object.path)/luna-settings.o: settings/settings.cpp

all.objects := $(libco.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(luna.objects)
all.options := $(libco.options) $(ruby.options) $(hiro.options) $(ares.options) $(luna.options) $(options)

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
	cp -R System/* $(prefix)/$(name)/
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	mkdir -p ~/Library/Application\ Support\$(name)/
	mkdir -p ~/Library/Application\ Support\$(name)/System/
	cp -R out/$(name).app /Applications/$(name).app
	cp -R System/* ~/Library/Application\ Support/$(name)/
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	cp out/$(name) $(prefix)/bin/$(name)
	cp -R $(ares.path)/System/* $(prefix)/share/$(name)/
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
