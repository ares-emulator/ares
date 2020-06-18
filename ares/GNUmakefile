ifeq ($(platform),windows)
  options += -luuid -lkernel32 -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32
  options += -Wl,-enable-auto-import
  options += -Wl,-enable-runtime-pseudo-reloc
else ifeq ($(platform),macos)
else ifneq ($(filter $(platform),linux bsd),)
  options += -Wl,-export-dynamic
  options += -lX11 -lXext
else
  $(error "unsupported platform")
endif

ares.objects := ares

$(object.path)/ares.o: $(ares.path)/ares/ares.cpp

ifeq ($(profile),accuracy)
  flags += -DPROFILE_ACCURACY
endif

ifeq ($(profile),performance)
  flags += -DPROFILE_PERFORMANCE
endif

ifneq ($(filter $(cores),fc),)
  include $(ares.path)/fc/GNUmakefile
endif

ifneq ($(filter $(cores),sfc),)
  include $(ares.path)/sfc/GNUmakefile
endif

ifneq ($(filter $(cores),n64),)
  include $(ares.path)/n64/GNUmakefile
endif

ifneq ($(filter $(cores),sg),)
  include $(ares.path)/sg/GNUmakefile
endif

ifneq ($(filter $(cores),ms),)
  include $(ares.path)/ms/GNUmakefile
endif

ifneq ($(filter $(cores),md),)
  include $(ares.path)/md/GNUmakefile
endif

ifneq ($(filter $(cores),ps1),)
  include $(ares.path)/ps1/GNUmakefile
endif

ifneq ($(filter $(cores),pce),)
  include $(ares.path)/pce/GNUmakefile
endif

ifneq ($(filter $(cores),msx),)
  include $(ares.path)/msx/GNUmakefile
endif

ifneq ($(filter $(cores),cv),)
  include $(ares.path)/cv/GNUmakefile
endif

ifneq ($(filter $(cores),gb),)
  include $(ares.path)/gb/GNUmakefile
endif

ifneq ($(filter $(cores),gba),)
  include $(ares.path)/gba/GNUmakefile
endif

ifneq ($(filter $(cores),ws),)
  include $(ares.path)/ws/GNUmakefile
endif

ifneq ($(filter $(cores),ngp),)
  include $(ares.path)/ngp/GNUmakefile
endif

include $(ares.path)/component/GNUmakefile

flags += $(foreach c,$(call strupper,$(cores)),-DCORE_$c)

ares.objects := $(ares.objects:%=$(object.path)/%.o)
