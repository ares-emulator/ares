PREFIX := /usr
INSTALLDIR := $(PREFIX)/share/libretro/shaders/shaders_slang

all:
	@echo "Nothing to make for slang-shaders."

install:
	mkdir -p $(DESTDIR)$(INSTALLDIR)
	cp -ar -t $(DESTDIR)$(INSTALLDIR) *
	rm -f $(DESTDIR)$(INSTALLDIR)/Makefile \
		$(DESTDIR)$(INSTALLDIR)/configure

test-install: all
	DESTDIR=/tmp/build $(MAKE) install
