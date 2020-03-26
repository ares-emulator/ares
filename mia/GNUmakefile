name := mia
build := performance
flags += -I. -I.. -I../ares

nall.path := ../nall
include $(nall.path)/GNUmakefile

hiro.path := ../hiro
hiro.resource := resource/$(name).rc
include $(hiro.path)/GNUmakefile

objects := obj/mia.o obj/resource.o

obj/mia.o: mia.cpp
obj/resource.o: resource/resource.cpp

all: $(hiro.objects) $(objects)
	$(info Linking out/$(name) ...)
	+@$(compiler) -o out/$(name) $(hiro.objects) $(objects) $(hiro.options) $(options)
ifeq ($(platform),macos)
	rm -rf out/$(name).app
	mkdir -p out/$(name).app/Contents/MacOS/
	mkdir -p out/$(name).app/Contents/Resources/
	mv out/$(name) out/$(name).app/Contents/MacOS/$(name)
	cp resource/$(name).plist out/$(name).app/Contents/Info.plist
	sips -s format icns resource/$(name).png --out out/$(name).app/Contents/Resources/$(name).icns
endif

verbose: hiro.verbose nall.verbose all;

clean:
ifeq ($(platform),macos)
	rm -rf out/$(name).app
endif
	$(call delete,obj/*)
	$(call delete,out/*)

install: all
ifeq ($(platform),windows)
	mkdir -p $(prefix)/$(name)/
	mkdir -p $(prefix)/$(name)/Database/
	mkdir -p $(prefix)/$(name)/Firmware/
	cp -R Database/* $(prefix)/$(name)/Database/
	cp -R Firmware/* $(prefix)/$(name)/Firmware/
else ifeq ($(shell id -un),root)
	$(error "make install should not be run as root")
else ifeq ($(platform),macos)
	cp -R out/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	mkdir -p $(prefix)/share/$(name)/Database/
	mkdir -p $(prefix)/share/$(name)/Firmware/
	cp out/$(name) $(prefix)/bin/$(name)
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

-include obj/*.d
