name := ares
build := optimized
threaded := true
openmp := false
vulkan := false
mame.rdp := true
local := true
flags += -I. -I.. -I../ares -I../thirdparty -DMIA_LIBRARY

nall.path := ../nall
include $(nall.path)/GNUmakefile

ifeq ($(platform),windows)
 #vulkan := $(if $(shell if exist ..\ares\n64\vulkan\parallel-rdp) echo 1),true,false)
  vulkan := $(if $(shell test -e ../ares/n64/vulkan/parallel-rdp && echo 1),true,false)
else ifeq ($(platform),linux)
  vulkan := $(if $(shell test -e ../ares/n64/vulkan/parallel-rdp && echo 1),true,false)
endif

ifeq ($(local),true)
  ifeq ($(arch),amd64)
    flags += -march=native
  endif
endif

libco.path := ../libco
include $(libco.path)/GNUmakefile

thirdparty.path := ../thirdparty
sljit.path := $(thirdparty.path)/sljit/sljit_src
include $(thirdparty.path)/GNUmakefile

ruby.path := ../ruby
include $(ruby.path)/GNUmakefile

hiro.path := ../hiro
hiro.resource := resource/ares.rc
include $(hiro.path)/GNUmakefile

profile := performance
cores := fc sfc n64 sg ms md ps1 pce ng msx cv gb gba ws ngp
#cores += saturn

ares.path := ../ares
include $(ares.path)/GNUmakefile

mia.path := ../mia

mia.objects := mia mia-resource
mia.objects := $(mia.objects:%=$(object.path)/%.o)

$(object.path)/mia.o: $(mia.path)/mia.cpp
$(object.path)/mia-resource.o: $(mia.path)/resource/resource.cpp

desktop-ui.path = ../desktop-ui

desktop-ui.objects += desktop-ui desktop-ui-resource desktop-ui-input desktop-ui-emulator desktop-ui-program
desktop-ui.objects += desktop-ui-presentation desktop-ui-settings desktop-ui-tools
desktop-ui.objects := $(desktop-ui.objects:%=$(object.path)/%.o)

$(object.path)/desktop-ui.o: $(desktop-ui.path)/desktop-ui.cpp
$(object.path)/desktop-ui-resource.o: $(desktop-ui.path)/resource/resource.cpp
$(object.path)/desktop-ui-input.o: $(desktop-ui.path)/input/input.cpp
$(object.path)/desktop-ui-emulator.o: $(desktop-ui.path)/emulator/emulator.cpp
$(object.path)/desktop-ui-program.o: $(desktop-ui.path)/program/program.cpp
$(object.path)/desktop-ui-presentation.o: $(desktop-ui.path)/presentation/presentation.cpp
$(object.path)/desktop-ui-settings.o: $(desktop-ui.path)/settings/settings.cpp
$(object.path)/desktop-ui-tools.o: $(desktop-ui.path)/tools/tools.cpp

all.objects := $(libco.objects) $(sljit.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(mia.objects) $(desktop-ui.objects)
all.options := $(libco.options) $(sljit.options) $(ruby.options) $(hiro.options) $(ares.options) $(mia.options) $(desktop-ui.options) $(options)

all: $(all.objects)
	$(info Linking $(output.path)/$(name) ...)
	+@$(compiler) -o $(output.path)/$(name) $(all.objects) $(all.options)
	cp ../LICENSE $(output.path)/LICENSE.txt
ifeq ($(platform),macos)
# Apply workaround for buggy linker in Xcode < 11.4.1
	@$(compiler) -o $(output.path)/macos-fix-jit macos-fix-jit.cpp
	$(output.path)/macos-fix-jit $(output.path)/$(name)
	rm -rf $(output.path)/$(name).app
	mkdir -p $(output.path)/$(name).app/Contents/MacOS/
	mkdir -p $(output.path)/$(name).app/Contents/Resources/
	mv $(output.path)/$(name) $(output.path)/$(name).app/Contents/MacOS/$(name)
	cp resource/$(name).plist $(output.path)/$(name).app/Contents/Info.plist
	cp -R $(ares.path)/Shaders $(output.path)/$(name).app/Contents/Resources/
	cp -R $(mia.path)/Database $(output.path)/$(name).app/Contents/Resources/	
	sips -s format icns resource/$(name).png --out $(output.path)/$(name).app/Contents/Resources/$(name).icns
else ifeq ($(platform),windows)
	cp -R $(ares.path)/Shaders $(output.path)/
	cp -R $(mia.path)/Database $(output.path)/
endif

verbose: nall.verbose ruby.verbose hiro.verbose all;

clean:
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
endif
	$(call rdelete,$(object.path)/*)
	$(call rdelete,$(output.path)/*)

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
	mkdir -p $(prefix)/share/icons/hicolor/256x256/apps/
	mkdir -p $(prefix)/share/$(name)/Shaders/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
	cp -R $(ares.path)/Shaders/* $(prefix)/share/$(name)/Shaders/
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
