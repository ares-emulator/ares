PKG_CONFIG ?= pkg-config

ifeq ($(platform),windows)
  ifeq ($(hiro),)
    hiro := windows
  endif

  ifeq ($(hiro),windows)
    hiro.flags   = $(flags.cpp) -DHIRO_WINDOWS
    hiro.options = $(call lib,kernel32 user32 gdi32 advapi32 ole32 comctl32 comdlg32 uxtheme msimg32 shlwapi dwmapi)
  endif

  ifeq ($(hiro),gtk2)
    hiro.flags   = $(flags.cpp) -DHIRO_GTK=2 $(shell pkg-config --cflags gtk+-2.0) -Wno-deprecated-declarations
    hiro.options = $(shell pkg-config --libs gtk+-2.0)
  endif

  ifeq ($(hiro),gtk3)
    hiro.flags   = $(flags.cpp) -DHIRO_GTK=3 $(shell pkg-config --cflags gtk+-3.0) -Wno-deprecated-declarations
    hiro.options = $(shell pkg-config --libs gtk+-3.0)
  endif
endif

ifeq ($(platform),macos)
  ifeq ($(hiro),)
    hiro := cocoa
  endif

  ifeq ($(hiro),cocoa)
    hiro.flags   = $(flags.objcpp) -w -DHIRO_COCOA
    hiro.options = -framework Cocoa -framework Carbon -framework IOKit -framework Security
  endif
endif

ifneq ($(filter $(platform),linux bsd),)
  ifeq ($(hiro),)
    hiro := gtk2
  endif

  ifeq ($(hiro),gtk2)
    hiro.flags   = $(flags.cpp) -DHIRO_GTK=2 $(shell $(PKG_CONFIG) --cflags gtk+-2.0) -Wno-deprecated-declarations
    hiro.options = -L/usr/local/lib -lX11 $(shell $(PKG_CONFIG) --libs gtk+-2.0)
  endif

  ifeq ($(hiro),gtk2-se)
    flags       += -DHiro_SourceEdit
    hiro.flags   = $(flags.cpp) -DHIRO_GTK=2 $(shell $(PKG_CONFIG) --cflags gtk+-2.0 gtksourceview-2.0) -Wno-deprecated-declarations
    hiro.options = -L/usr/local/lib -lX11 $(shell $(PKG_CONFIG) --libs gtk+-2.0 gtksourceview-2.0)
  endif

  ifeq ($(hiro),gtk3)
    hiro.flags   = $(flags.cpp) -DHIRO_GTK=3 $(shell $(PKG_CONFIG) --cflags gtk+-3.0) -Wno-deprecated-declarations
    hiro.options = -L/usr/local/lib -lX11 $(shell $(PKG_CONFIG) --libs gtk+-3.0)
  endif

  ifeq ($(hiro),gtk3-se)
    flags       += -DHiro_SourceEdit
    hiro.flags   = $(flags.cpp) -DHIRO_GTK=3 $(shell $(PKG_CONFIG) --cflags gtk+-3.0 gtksourceview-3.0) -Wno-deprecated-declarations
    hiro.options = -L/usr/local/lib -lX11 $(shell $(PKG_CONFIG) --libs gtk+-3.0 gtksourceview-3.0)
  endif

  ifeq ($(hiro),qt4)
    moc = /usr/local/lib/qt4/bin/moc
    hiro.flags   = $(flags.cpp) -DHIRO_QT=4 $(shell $(PKG_CONFIG) --cflags QtCore QtGui)
    hiro.options = -L/usr/local/lib -lX11 $(shell $(PKG_CONFIG) --libs QtCore QtGui)
  endif

  ifeq ($(hiro),qt5)
    moc = $(shell $(PKG_CONFIG) --variable=host_bins Qt5Core)/moc
    hiro.flags   = $(flags.cpp) -DHIRO_QT=5 -fPIC $(shell $(PKG_CONFIG) --cflags Qt5Core Qt5Gui Qt5Widgets)
    hiro.options = -L/usr/local/lib -lX11 $(shell $(PKG_CONFIG) --libs Qt5Core Qt5Gui Qt5Widgets)
  endif
endif

ifeq ($(hiro.resource),)
  hiro.resource := $(hiro.path)/windows/hiro.rc
endif

ifneq ($(findstring windres,$(windres)),)
  # windres
  hiro.resource.extension := .o
  hiro.resource.command = $1 $2
else
  # rc
  hiro.resource.extension := .res
  hiro.resource.command = /nologo /fo $2 $1
endif

hiro.objects := \
  $(object.path)/hiro-$(hiro).o \
  $(if $(filter windows,$(hiro)),$(object.path)/hiro-resource$(hiro.resource.extension))

$(object.path)/hiro-$(hiro).o: $(hiro.path)/hiro.cpp
	$(if $(filter qt%,$(hiro)),$(info Compiling $(hiro.path)/qt/qt.moc ...))
	$(if $(filter qt%,$(hiro)),@$(moc) -i -o $(hiro.path)/qt/qt.moc $(hiro.path)/qt/qt.hpp)
	$(info Compiling $(subst ../,,$<) ...)
	@$(compiler) $(hiro.flags) $(flags) $(flags.deps) -c $< $(call obj,$@)

$(object.path)/hiro-resource$(hiro.resource.extension): $(hiro.resource)
	$(info Compiling $(subst ../,,$<) ...)
	@$(windres) $(call hiro.resource.command,$<,$@)

hiro.verbose:
	$(info hiro Target:)
	$(info $([space]) $(hiro))
