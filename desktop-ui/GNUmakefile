name := ares
build := optimized
threaded := true
openmp := false
vulkan := true
sdl2 := true
local := true
lto := true
librashader	:= true
flags += -I. -I.. -I../ares -I../thirdparty -I../thirdparty/librashader/include -DMIA_LIBRARY

nall.path := ../nall
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

ifeq ($(platform),macos)
  ares.dylibs :=
endif

libco.path := ../libco
include $(libco.path)/GNUmakefile

thirdparty.path := ../thirdparty
sljit.path := $(thirdparty.path)/sljit/sljit_src
libchdr.path := $(thirdparty.path)/libchdr
tzxfile.path := $(thirdparty.path)/TZXFile
ymfm.path := $(thirdparty.path)/ymfm
include $(thirdparty.path)/GNUmakefile

ifeq ($(platform),macos)
	librashader.path := $(thirdparty.path)/librashader/target/optimized/librashader.dylib
else ifeq ($(platform), windows)
  ifneq ($(wildcard $(thirdparty.path)/librashader/target/aarch64-pc-windows-msvc/optimized/librashader.dll),)
	librashader.path := $(thirdparty.path)/librashader/target/aarch64-pc-windows-msvc/optimized/librashader.dll
  else
	librashader.path := $(thirdparty.path)/librashader/target/optimized/librashader.dll
  endif
endif

ruby.path := ../ruby
include $(ruby.path)/GNUmakefile

hiro.path := ../hiro
hiro.resource := resource/ares.rc
include $(hiro.path)/GNUmakefile

profile := performance
cores := a26 fc sfc n64 sg ms md ps1 pce ng msx cv myvision gb gba ws ngp spec
#cores += saturn

ares.path := ../ares
include $(ares.path)/GNUmakefile

mia.path := ../mia

mia.objects := mia mia-resource
mia.objects := $(mia.objects:%=$(object.path)/%.o)

$(object.path)/mia.o: $(mia.path)/mia.cpp
$(object.path)/mia-resource.o: $(mia.path)/resource/resource.cpp

desktop-ui.path = ../desktop-ui

desktop-ui.objects += desktop-ui desktop-ui-resource desktop-ui-input desktop-ui-game-browser desktop-ui-emulator
desktop-ui.objects += desktop-ui-program desktop-ui-presentation desktop-ui-settings desktop-ui-tools
desktop-ui.objects := $(desktop-ui.objects:%=$(object.path)/%.o)

$(object.path)/desktop-ui.o: $(desktop-ui.path)/desktop-ui.cpp
$(object.path)/desktop-ui-resource.o: $(desktop-ui.path)/resource/resource.cpp
$(object.path)/desktop-ui-input.o: $(desktop-ui.path)/input/input.cpp
$(object.path)/desktop-ui-emulator.o: $(desktop-ui.path)/emulator/emulator.cpp
$(object.path)/desktop-ui-game-browser.o: $(desktop-ui.path)/game-browser/game-browser.cpp
$(object.path)/desktop-ui-program.o: $(desktop-ui.path)/program/program.cpp
$(object.path)/desktop-ui-presentation.o: $(desktop-ui.path)/presentation/presentation.cpp
$(object.path)/desktop-ui-settings.o: $(desktop-ui.path)/settings/settings.cpp
$(object.path)/desktop-ui-tools.o: $(desktop-ui.path)/tools/tools.cpp

all.objects :=            $(libco.objects) $(sljit.objects) $(libchdr.objects) $(tzxfile.objects) $(ymfm.objects) $(nall.objects) $(ruby.objects) $(hiro.objects) $(ares.objects) $(mia.objects) $(desktop-ui.objects)
all.options := $(options) $(libco.options) $(sljit.options) $(libchdr.options) $(tzxfile.options) $(ymfm.options) $(nall.options) $(ruby.options) $(hiro.options) $(ares.options) $(mia.options) $(desktop-ui.options)

$(all.objects): | $(object.path)

all: output
ifneq ($(MAKECMDGOALS),clean)
   ifeq ($(platform),macos)
      ifeq ($(librashader), true)
        ifeq ($(wildcard $(librashader.path)),)
    	  $(error Tried to compile ares for macOS with librashader enabled, but no librashader library was found. Compile it with thirdparty/librashader/build-librashader.sh, or disable librashader by compiling ares with librashader=false)
        endif
    	ares.dylibs += $(librashader.path)
      endif
   else ifeq ($(platform), windows)
      ifeq ($(librashader), true)
        ifeq ($(wildcard $(librashader.path)),)
          $(error Tried to compile ares for Windows with librashader enabled, but no librashader library was found. Compile it with thirdparty/librashader/build-librashader.sh, or disable librashader by compiling ares with librashader=false)
        endif
      endif
   endif
endif

output: $(all.objects) | $(output.path)
	$(info Linking $(output.path)/$(name)$(extension) ...)
	+@$(compiler) $(call exe,$(output.path)/$(name)$(extension)) $(all.objects) $(all.options)
	$(call copy,../LICENSE,$(output.path)/LICENSE.txt)
ifeq ($(platform),macos)
# Apply workaround for buggy linker in Xcode < 11.4.1
	@$(compiler) -o $(output.path)/macos-fix-jit macos-fix-jit.cpp
	$(output.path)/macos-fix-jit $(output.path)/$(name)
	rm -f $(output.path)/macos-fix-jit
	rm -rf $(output.path)/$(name).app
	install_name_tool -add_rpath @executable_path/../Frameworks $(output.path)/$(name)
	mkdir -p $(output.path)/$(name).app/Contents/MacOS/
	mkdir -p $(output.path)/$(name).app/Contents/Resources/
	mkdir -p $(output.path)/$(name).app/Contents/Frameworks/
ifneq ($(ares.dylibs),)
	cp $(ares.dylibs) $(output.path)/$(name).app/Contents/Frameworks/
endif
	mv $(output.path)/$(name) $(output.path)/$(name).app/Contents/MacOS/$(name)
	cp resource/$(name).plist $(output.path)/$(name).app/Contents/Info.plist
	$(call mkdir,$(output.path)/$(name).app/Contents/Resources/Shaders/)
	$(call mkdir,$(output.path)/$(name).app/Contents/Resources/Database/)
ifeq ($(build),debug)
# Try to copy the debug .metallib into the .app bundle; if we don't succeed, user will see runtime log message
	-cp ../ruby/video/metal/shaders.metallib $(output.path)/$(name).app/Contents/Resources/Shaders/shaders.metallib
endif
	cp ../ruby/video/metal/Shaders.metal $(output.path)/$(name).app/Contents/Resources/Shaders/Shaders.metal
	$(call rcopy,$(thirdparty.path)/slang-shaders/*,$(output.path)/$(name).app/Contents/Resources/Shaders/)
	$(call rcopy,$(mia.path)/Database/*,$(output.path)/$(name).app/Contents/Resources/Database/)
	sips -s format icns resource/$(name).png --out $(output.path)/$(name).app/Contents/Resources/$(name).icns
	codesign --force --deep --options runtime --entitlements resource/$(name).selfsigned.entitlements --sign - $(output.path)/$(name).app
else ifneq ($(filter $(platform),windows linux bsd),)
	$(call mkdir,$(output.path)/Shaders/)
	$(call mkdir,$(output.path)/Database/)
	$(call rcopy,$(thirdparty.path)/slang-shaders/*,$(output.path)/Shaders/)
	$(call rcopy,$(mia.path)/Database/*,$(output.path)/Database/)
ifeq ($(librashader), true)
ifdef librashader.path
	$(call copy,$(librashader.path),$(output.path)/)
endif
endif
endif

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
	cp -R $(thirdparty.path)/slang-shaders/* $(prefix)/share/$(name)/Shaders/
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
