ifeq ($(ruby),)
  ifeq ($(platform),windows)
    ruby += video.direct3d9 video.gdi
    ifeq ($(arch),amd64)
      ruby += video.wgl
    endif
    ruby += audio.wasapi audio.xaudio2 audio.directsound audio.waveout #audio.asio
    ruby += input.windows
  else ifeq ($(platform),macos)
    ruby += video.cgl
    ruby += audio.openal
    ruby += input.quartz #input.carbon
  else ifeq ($(platform),linux)
    pkg_check = $(if $(shell pkg-config $1 && echo 1),$2)
    ruby += video.glx video.glx2 video.xshm
    ruby += $(call pkg_check,xv,video.xvideo)
    ruby += audio.oss audio.alsa
    ruby += $(call pkg_check,openal,audio.openal)
    ruby += $(call pkg_check,libpulse,audio.pulseaudio)
    ruby += $(call pkg_check,libpulse-simple,audio.pulseaudiosimple)
    ruby += $(call pkg_check,ao,audio.ao)
    ruby += input.xlib
    ruby += $(call pkg_check,libudev,input.udev)
    ruby += $(call pkg_check,sdl2,input.sdl)
  else ifeq ($(platform),bsd)
    pkg_check = $(if $(shell pkg-config $1 && echo 1),$2)
    ruby += video.glx video.glx2 video.xshm
    ruby += $(call pkg_check,xv,video.xvideo)
    ruby += audio.oss
    ruby += $(call pkg_check,openal,audio.openal)
    ruby += $(call pkg_check,libpulse,audio.pulseaudio)
    ruby += $(call pkg_check,libpulse-simple,audio.pulseaudiosimple)
    ruby += $(call pkg_check,ao,audio.ao)
    ruby += input.uhid input.xlib
    ruby += $(call pkg_check,sdl2,input.sdl)
  endif
endif

ifeq ($(platform),macos)
  ruby.flags := $(flags.objcpp)
else
  ruby.flags := $(flags.cpp)
endif

ruby.flags += $(foreach c,$(subst .,_,$(call strupper,$(ruby))),-D$c)
ruby.flags += $(if $(findstring input.sdl,$(ruby)),$(shell sdl2-config --cflags))

ruby.options :=

ruby.options += $(if $(findstring video.cgl,$(ruby)),-framework OpenGL)
ruby.options += $(if $(findstring video.direct3d9,$(ruby)),$(call lib,d3d9))
ruby.options += $(if $(findstring video.glx,$(ruby)),-lGL)
ruby.options += $(if $(findstring video.wgl,$(ruby)),$(call lib,opengl32))
ruby.options += $(if $(findstring video.xvideo,$(ruby)),-lXv)

ruby.options += $(if $(findstring audio.alsa,$(ruby)),-lasound)
ruby.options += $(if $(findstring audio.ao,$(ruby)),-lao)
ruby.options += $(if $(findstring audio.directsound,$(ruby)),$(call lib,dsound uuid))
ruby.options += $(if $(findstring audio.pulseaudio,$(ruby)),-lpulse)
ruby.options += $(if $(findstring audio.pulseaudiosimple,$(ruby)),-lpulse-simple)
ruby.options += $(if $(findstring audio.wasapi,$(ruby)),$(call lib,avrt uuid))
ruby.options += $(if $(findstring audio.waveout,$(ruby)),$(call lib,winmm))
ruby.options += $(if $(findstring audio.xaudio2,$(ruby)),$(call lib,ole32))

ruby.options += $(if $(findstring input.sdl,$(ruby)),$(shell sdl2-config --libs))
ruby.options += $(if $(findstring input.udev,$(ruby)),-ludev)
ruby.options += $(if $(findstring input.uhid,$(ruby)),-lusbhid)
ruby.options += $(if $(findstring input.windows,$(ruby)),$(call lib,dinput8 dxguid))

ifeq ($(platform),windows)
  ruby.options += $(if $(findstring audio.openal,$(ruby)),$(call lib,openal32))
endif

ifeq ($(platform),macos)
  ruby.options += -framework IOKit
  ruby.options += $(if $(findstring audio.openal,$(ruby)),-framework OpenAL)
endif

ifeq ($(platform),linux)
  ruby.options += -lX11 -lXext -lXrandr
  ruby.options += $(if $(findstring audio.openal,$(ruby)),-lopenal)
endif

ifeq ($(platform),bsd)
  ruby.options += -lX11 -lXext -lXrandr
  ruby.options += $(if $(findstring audio.openal,$(ruby)),-lopenal -fuse-ld=bfd)
  # -fuse-ld=bfd: see FreeBSD bug 219089
endif

ruby.objects := $(object.path)/ruby.o

$(object.path)/ruby.o: $(ruby.path)/ruby.cpp $(call rwildcard,$(ruby.path))
	$(info Compiling $(subst ../,,$<) ...)
	@$(compiler) $(ruby.flags) $(flags) $(flags.deps) -c $< $(call obj,$@)

ruby.verbose:
	$(info ruby Drivers:)
	$(foreach n,$(ruby),$(info $([space]) $n))
