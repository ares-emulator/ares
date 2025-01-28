ifeq ($(platform),windows)
  options += $(call lib,uuid kernel32 user32 gdi32 comctl32 comdlg32 shell32)
else ifeq ($(platform),macos)
else ifneq ($(filter $(platform),linux bsd),)
  options += -Wl,-export-dynamic
  options += -lX11 -lXext
else
  $(error "ares is a library and cannot be built directly.")
endif

ares.objects := ares ares-fixed-allocator

$(object.path)/ares.o: $(ares.path)/ares/ares.cpp
$(object.path)/ares-fixed-allocator.o: $(ares.path)/ares/memory/fixed-allocator.cpp

ifeq ($(vulkan),true)
  flags += -DVULKAN
endif

ifeq ($(profile),accuracy)
  flags += -DPROFILE_ACCURACY
endif

ifeq ($(profile),performance)
  flags += -DPROFILE_PERFORMANCE
endif

ifneq ($(filter $(cores),a26),)
  include $(ares.path)/a26/GNUmakefile
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

ifneq ($(filter $(cores),saturn),)
  include $(ares.path)/saturn/GNUmakefile
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

ifneq ($(filter $(cores),pencil2),)
  include $(ares.path)/pencil2/GNUmakefile
endif

ifneq ($(filter $(cores),myvision),)
  include $(ares.path)/myvision/GNUmakefile
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

ifneq ($(filter $(cores),ng),)
  include $(ares.path)/ng/GNUmakefile
endif

ifneq ($(filter $(cores),ngp),)
  include $(ares.path)/ngp/GNUmakefile
endif

ifneq ($(filter $(cores),spec),)
  include $(ares.path)/spec/GNUmakefile
endif

include $(ares.path)/component/GNUmakefile

flags += $(foreach c,$(call strupper,$(cores)),-DCORE_$c)

ares.objects := $(ares.objects:%=$(object.path)/%.o)
