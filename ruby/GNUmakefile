ifeq ($(ruby),)
  ifeq ($(platform),windows)
    ruby += video.direct3d9 video.gdi
    ifneq ($(filter $(arch),x86 amd64),)
      ruby += video.wgl
    endif
    ruby += audio.wasapi audio.xaudio2 audio.directsound audio.waveout #audio.asio
    ruby += input.windows

    ifeq ($(pkg_config),)
      # TODO: Check presence of libSDL
    else
      pkg_check = $(if $(shell $(pkg_config) $1 && echo 1),$2)
      ifeq ($(sdl2),true)
        ruby += $(call pkg_check,sdl2,input.sdl)
        ruby += $(call pkg_check,sdl2,audio.sdl)
      endif
    endif
  else ifeq ($(platform),macos)
    ruby += video.cgl
    ruby += audio.openal
    ruby += input.quartz #input.carbon
    ruby += video.metal
    ifeq ($(sdl2),true)
      macsdl = ../thirdparty/SDL/libSDL2-2.0.0.dylib
      ares.dylibs += $(macsdl)
      ifeq ($(wildcard $(macsdl)),)
        $(error Tried to compile ruby for macOS with SDL2 linked, but no SDL2 library was found. Compile it with thirdparty/SDL/build-sdl.sh, or disable SDL by compiling ares with sdl2=false)
      endif
      flags += -DMACOS_COMPILED_SDL
      ruby += audio.sdl
      ruby += input.sdl
    endif
  else ifeq ($(platform),linux)
    pkg_check = $(if $(shell $(pkg_config) $1 && echo 1),$2)
    ruby += video.glx video.xshm
    ruby += $(call pkg_check,xv,video.xvideo)
    ruby += audio.oss audio.alsa
    ruby += $(call pkg_check,openal,audio.openal)
    ruby += $(call pkg_check,libpulse,audio.pulseaudio)
    ruby += $(call pkg_check,libpulse-simple,audio.pulseaudiosimple)
    ruby += $(call pkg_check,ao,audio.ao)
    ruby += input.xlib
    ruby += $(call pkg_check,libudev,input.udev)
    ifeq ($(sdl2),true)
      ruby += $(call pkg_check,sdl2,input.sdl)
      ruby += $(call pkg_check,sdl2,audio.sdl)
    endif
  else ifeq ($(platform),bsd)
    pkg_check = $(if $(shell $(pkg_config) $1 && echo 1),$2)
    ruby += video.glx video.glx2 video.xshm
    ruby += $(call pkg_check,xv,video.xvideo)
    ruby += audio.oss
    ruby += $(call pkg_check,openal,audio.openal)
    ruby += $(call pkg_check,libpulse,audio.pulseaudio)
    ruby += $(call pkg_check,libpulse-simple,audio.pulseaudiosimple)
    ruby += $(call pkg_check,ao,audio.ao)
    ruby += input.uhid input.xlib
    ifeq ($(sdl2),true)
      ruby += $(call pkg_check,sdl2,input.sdl)
      ruby += $(call pkg_check,sdl2,audio.sdl)
    endif
  endif
endif

ifeq ($(platform),macos)
  ruby.flags := $(flags.objcpp)
  ifeq ($(sdl2),true)
    ruby.flags += -I ../thirdparty/SDL/SDL/include/
  endif
else
  ruby.flags := $(flags.cpp)
endif

ruby.flags += -I../thirdparty
ruby.flags += $(foreach c,$(subst .,_,$(call strupper,$(ruby))),-D$c)
ifeq ($(pkg_config),)
  # TODO: add SDL2 cflags
else ifeq ($(wildcard $(macsdl)),)
  ruby.flags += $(if $(findstring input.sdl,$(ruby)),$(shell $(pkg_config) sdl2 --cflags))
  ruby.flags += $(if $(findstring audio.sdl,$(ruby)),$(shell $(pkg_config) sdl2 --cflags))
endif

ruby.options :=

ruby.options += $(if $(findstring video.cgl,$(ruby)),-framework OpenGL)
ruby.options += $(if $(findstring video.direct3d9,$(ruby)),$(call lib,d3d9))
ruby.options += $(if $(findstring video.glx,$(ruby)),-lGL)
ruby.options += $(if $(findstring video.wgl,$(ruby)),$(call lib,opengl32 dwmapi))
ruby.options += $(if $(findstring video.xvideo,$(ruby)),-lXv)

ruby.options += $(if $(findstring audio.alsa,$(ruby)),-lasound)
ruby.options += $(if $(findstring audio.ao,$(ruby)),-lao)
ruby.options += $(if $(findstring audio.directsound,$(ruby)),$(call lib,dsound uuid))
ruby.options += $(if $(findstring audio.pulseaudio,$(ruby)),-lpulse)
ruby.options += $(if $(findstring audio.pulseaudiosimple,$(ruby)),-lpulse-simple)
ruby.options += $(if $(findstring audio.wasapi,$(ruby)),$(call lib,avrt uuid))
ruby.options += $(if $(findstring audio.waveout,$(ruby)),$(call lib,winmm))
ruby.options += $(if $(findstring audio.xaudio2,$(ruby)),$(call lib,ole32))

ifeq ($(platform),windows)
  ifeq ($(pkg_config),)
    # TODO: add SDL2 ldflags
  else
    ruby.options += $(if $(findstring input.sdl,$(ruby)),$(shell $(pkg_config) sdl2 --libs --static))
    ruby.options += $(if $(findstring audio.sdl,$(ruby)),$(shell $(pkg_config) sdl2 --libs --static))
  endif
else ifeq ($(wildcard $(macsdl)),)
  ruby.options += $(if $(findstring input.sdl,$(ruby)),$(shell $(pkg_config) sdl2 --libs))
  ruby.options += $(if $(findstring audio.sdl,$(ruby)),$(shell $(pkg_config) sdl2 --libs))
endif

ruby.options += $(if $(findstring input.udev,$(ruby)),-ludev)
ruby.options += $(if $(findstring input.uhid,$(ruby)),-lusbhid)
ruby.options += $(if $(findstring input.windows,$(ruby)),$(call lib,dinput8 dxguid))

ifeq ($(platform),windows)
  ruby.options += $(if $(findstring audio.openal,$(ruby)),$(call lib,openal32))
endif

ifeq ($(platform),macos)
  ruby.options += -framework Metal
  ruby.options += -framework MetalKit
  ruby.options += -framework IOKit
  ruby.options += $(if $(findstring audio.openal,$(ruby)),-framework OpenAL)
  ifeq ($(sdl2),true)
    ruby.options += -L../thirdparty/SDL/ -lSDL2-2.0.0
  endif
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
