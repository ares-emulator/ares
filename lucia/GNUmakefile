name := lucia
build := performance
openmp := false
local := true
flags += -I. -I.. -I../ares -DMIA_LIBRARY

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

mia.path := ../mia

mia.objects := mia mia-resource
mia.objects := $(mia.objects:%=$(object.path)/%.o)

$(object.path)/mia.o: $(mia.path)/mia.cpp
$(object.path)/mia-resource.o: $(mia.path)/resource/resource.cpp

lucia.path = ../lucia

lucia.objects += lucia lucia-resource lucia-emulator lucia-program lucia-input
lucia.objects += lucia-presentation lucia-settings lucia-tools
lucia.objects := $(lucia.objects:%=$(object.path)/%.o)

$(object.path)/lucia.o: $(lucia.path)/lucia.cpp
$(object.path)/lucia-resource.o: $(lucia.path)/resource/resource.cpp
$(object.path)/lucia-emulator.o: $(lucia.path)/emulator/emulator.cpp
$(object.path)/lucia-program.o: $(lucia.path)/program/program.cpp
$(object.path)/lucia-input.o: $(lucia.path)/input/input.cpp
$(object.path)/lucia-presentation.o: $(lucia.path)/presentation/presentation.cpp
$(object.path)/lucia-settings.o: $(lucia.path)/settings/settings.cpp
$(object.path)/lucia-tools.o: $(lucia.path)/tools/tools.cpp

all.objects := $(libco.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(mia.objects) $(lucia.objects)
all.options := $(libco.options) $(ruby.options) $(hiro.options) $(ares.options) $(mia.options) $(lucia.options) $(options)

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
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	mkdir -p ~/Library/Application\ Support/$(name)/
	cp -R $(output.path)/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
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
