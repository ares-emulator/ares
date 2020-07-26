name := luna
build := performance
openmp := false
local := true
flags += -I. -I.. -I../ares

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
hiro.resource := resource/luna.rc
include $(hiro.path)/GNUmakefile

profile := accuracy
cores := fc sfc n64 sg ms md ps1 pce msx cv gb gba ws ngp

ares.path := ../ares
include $(ares.path)/GNUmakefile

luna.path := ../luna

luna.objects += luna luna-resource luna-emulator
luna.objects += luna-input luna-menus luna-program luna-settings
luna.objects := $(luna.objects:%=$(object.path)/%.o)

$(object.path)/luna.o: $(luna.path)/luna.cpp
$(object.path)/luna-resource.o: $(luna.path)/resource/resource.cpp
$(object.path)/luna-emulator.o: $(luna.path)/emulator/emulator.cpp
$(object.path)/luna-input.o: $(luna.path)/input/input.cpp
$(object.path)/luna-menus.o: $(luna.path)/menus/menus.cpp
$(object.path)/luna-program.o: $(luna.path)/program/program.cpp
$(object.path)/luna-settings.o: $(luna.path)/settings/settings.cpp

all.objects := $(libco.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(luna.objects)
all.options := $(libco.options) $(ruby.options) $(hiro.options) $(ares.options) $(luna.options) $(options)

all: $(all.objects)
	$(info Linking $(output.path)/$(name) ...)
	+@$(compiler) -o $(output.path)/$(name) $(all.objects) $(all.options)
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
	mkdir -p $(output.path)/$(name).app/Contents/MacOS/
	mkdir -p $(output.path)/$(name).app/Contents/Resources/
	mv $(output.path)/$(name) $(output.path)/$(name).app/Contents/MacOS/$(name)
	cp resource/$(name).plist $(output.path)/$(name).app/Contents/Info.plist
	sips -s format icns resource/$(name).png --out $(output.path)/$(name).app/Contents/Resources/$(name).icns
endif

verbose: nall.verbose ruby.verbose hiro.verbose all;

clean:
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
endif
	$(call delete,$(object.path)/*)
	$(call delete,$(output.path)/*)

install: all
ifeq ($(platform),windows)
	mkdir -p $(prefix)/$(name)/
	cp -R System/* $(prefix)/$(name)/
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	mkdir -p ~/Library/Application\ Support\$(name)/
	mkdir -p ~/Library/Application\ Support\$(name)/System/
	cp -R $(output.path)/$(name).app /Applications/$(name).app
	cp -R System/* ~/Library/Application\ Support/$(name)/System/
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
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

-include $(object.path)/*.d
