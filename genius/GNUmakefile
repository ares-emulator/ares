name := genius
build := stable
flags += -I..

nall.path := ../nall
include $(nall.path)/GNUmakefile

hiro.path := ../hiro
hiro.resource := data/$(name).rc
include $(hiro.path)/GNUmakefile

objects := $(object.path)/genius.o

$(object.path)/genius.o: genius.cpp

$(objects): | $(object.path)

all: $(hiro.objects) $(objects) | $(output.path)
	$(info Linking $(output.path)/$(name) ...)
	+@$(compiler) -o $(output.path)/$(name) $(hiro.objects) $(objects) $(hiro.options) $(options)
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
	mkdir -p $(output.path)/$(name).app/Contents/MacOS/
	mkdir -p $(output.path)/$(name).app/Contents/Resources/
	mv $(output.path)/$(name) $(output.path)/$(name).app/Contents/MacOS/$(name)
	cp data/$(name).plist $(output.path)/$(name).app/Contents/Info.plist
	sips -s format icns data/$(name).png --$(output.path) $(output.path)/$(name).app/Contents/Resources/$(name).icns
endif

verbose: hiro.verbose nall.verbose all;

clean:
ifeq ($(platform),macos)
	rm -rf $(output.path)/$(name).app
endif
	$(call delete,$(object.path)/*)
	$(call delete,$(output.path)/*)

install: all
ifeq ($(platform),macos)
	cp -R $(output.path)/$(name).app /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	mkdir -p $(prefix)/bin/
	mkdir -p $(prefix)/share/applications/
	mkdir -p $(prefix)/share/icons/
	mkdir -p $(prefix)/share/$(name)/
	cp $(output.path)/$(name) $(prefix)/bin/$(name)
	cp data/$(name).desktop $(prefix)/share/applications/$(name).desktop
	cp data/$(name).png $(prefix)/share/icons/$(name).png
endif

uninstall:
ifeq ($(platform),macos)
	rm -rf /Applications/$(name).app
else ifneq ($(filter $(platform),linux bsd),)
	rm -f $(prefix)/bin/$(name)
	rm -f $(prefix)/share/applications/$(name).desktop
	rm -f $(prefix)/share/icons/$(name).png
endif

-include $(object.path)/*.d
