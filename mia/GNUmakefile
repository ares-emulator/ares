name := mia
build := optimized
threaded := true
local := true
flags += -I. -I.. -I../ares -I../thirdparty

nall.path := ../nall
include $(nall.path)/GNUmakefile

thirdparty.path := ../thirdparty
libchdr.path := $(thirdparty.path)/libchdr
tzxfile.path := $(thirdparty.path)/TZXFile
include $(thirdparty.path)/GNUmakefile

hiro.path := ../hiro
hiro.resource := resource/$(name).rc
include $(hiro.path)/GNUmakefile

ares.path := ../ares
mia.path := ../mia

objects := ares mia resource
objects := $(objects:%=$(object.path)/%.o)

$(object.path)/ares.o: $(ares.path)/ares/ares.cpp
$(object.path)/mia.o: $(mia.path)/mia.cpp
$(object.path)/resource.o: $(mia.path)/resource/resource.cpp

all.objects := $(libchdr.objects) $(tzxfile.objects) $(nall.objects) $(hiro.objects) $(objects)
all.options := $(libchdr.options) $(tzxfile.options) $(nall.options) $(hiro.options) $(options)

$(all.objects): | $(object.path)

all: $(all.objects) | $(output.path)
	$(info Linking $(output.path)/$(name)$(extension) ...)
	+@$(compiler) -o $(output.path)/$(name)$(extension) $(all.objects) $(all.options)
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
	mkdir -p $(output.path)/$(name).app/Contents/MacOS/
	mkdir -p $(output.path)/$(name).app/Contents/Resources/
	mv $(output.path)/$(name) $(output.path)/$(name).app/Contents/MacOS/$(name)
	cp resource/$(name).plist $(output.path)/$(name).app/Contents/Info.plist
	sips -s format icns resource/$(name).png --out $(output.path)/$(name).app/Contents/Resources/$(name).icns
endif

verbose: hiro.verbose nall.verbose all;

clean:
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
endif
	$(call delete,$(object.path)/*)
	$(call delete,$(output.path)/*)

install: all
ifeq ($(platform),windows)
	$(call mkdir,$(prefix)/$(name)/)
	$(call mkdir,$(prefix)/$(name)/Database/)
	$(call mkdir,$(prefix)/$(name)/Firmware/)
	$(call rcopy,Database/*,$(prefix)/$(name)/Database/)
	$(call rcopy,Firmware/*,$(prefix)/$(name)/Firmware/)
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	cp -R $(output.path)/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	mkdir -p $(prefix)/share/$(name)/Database/
	mkdir -p $(prefix)/share/$(name)/Firmware/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
	cp resource/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp resource/$(name).png $(prefix)/share/icons/$(name).png
	cp -R Database/* $(prefix)/share/$(name)/Database/
	cp -R Firmware/* $(prefix)/share/$(name)/Firmware/
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
