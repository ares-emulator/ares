# disable built-in rules and variables
MAKEFLAGS := Rr
.SUFFIXES:

[0-9] = 0 1 2 3 4 5 6 7 8 9
[A-Z] = A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
[a-z] = a b c d e f g h i j k l m n o p q r s t u v w x y z
[markup] = ` ~ ! @ \# $$ % ^ & * ( ) - _ = + [ { ] } \ | ; : ' " , < . > / ?
[all] = $([0-9]) $([A-Z]) $([a-z]) $([markup])
[empty] :=
[space] := $([empty]) $([empty])

# platform detection
ifeq ($(platform),)
  ifeq ($(OS),Windows_NT)
    platform := windows
  endif
endif

ifeq ($(platform),)
  uname := $(shell uname)
  ifeq ($(uname),)
    platform := windows
  else ifneq ($(findstring Windows,$(uname)),)
    platform := windows
  else ifneq ($(findstring NT,$(uname)),)
    platform := windows
  else ifneq ($(findstring Darwin,$(uname)),)
    platform := macos
  else ifneq ($(findstring Linux,$(uname)),)
    platform := linux
  else ifneq ($(findstring BSD,$(uname)),)
    platform := bsd
  else
    $(error unknown platform, please specify manually.)
  endif
endif

# common commands
ifeq ($(shell echo ^^),^)
  # cmd
  fixpath = $(subst /,\\,$1)
  mkdir   = @if not exist $(call fixpath,$1) (mkdir $(call fixpath,$1))
  copy    = @copy $(call fixpath,$1) $(call fixpath,$2)
  rcopy   = @xcopy /e /q /y $(call fixpath,$1) $(call fixpath,$2)
  delete  = $(info Deleting $1 ...) @del /q $(call fixpath,$1)
  rdelete = $(info Deleting $1 ...) @if exist $(call fixpath,$1) (rmdir /s /q $(call fixpath,$1))
  which   = $(shell where $1 2> NUL)
else
  # sh
  mkdir   = @mkdir -p $1
  copy    = @cp $1 $2
  rcopy   = @cp -R $1 $2
  delete  = $(info Deleting $1 ...) @rm -f $1
  rdelete = $(info Deleting $1 ...) @rm -rf $1
  which   = $(shell which $1 2> /dev/null)
endif

ifeq ($(ccache), true)
  compiler.c      = ccache $(compiler) $(flags.c)
  compiler.cpp    = ccache $(compiler) $(flags.cpp)
  compiler.objc   = ccache $(compiler) $(flags.objc)
  compiler.objcpp = ccache $(compiler) $(flags.objcpp)
else
  compiler.c      = $(compiler) $(flags.c)
  compiler.cpp    = $(compiler) $(flags.cpp)
  compiler.objc   = $(compiler) $(flags.objc)
  compiler.objcpp = $(compiler) $(flags.objcpp)
endif

# compiler detection
ifeq ($(compiler),)
  ifneq ($(filter windows macos bsd,$(platform)),)
    compilers := clang++ g++
  else
    compilers := g++ clang++
  endif

  ifneq ($(call which,$(word 1, $(compilers))),)
    compiler := $(word 1, $(compilers))
  else ifneq ($(call which,$(word 2, $(compilers))),)
    compiler := $(word 2, $(compilers))
  else
    $(error unknown compiler, please specify manually.)
  endif
endif

ifneq ($(filter cl clang-cl,$(compiler)),)
  cl := true
  msvc := true
endif

ifneq ($(filter cl,$(compiler)),)
  machine_str := $(shell $(compiler) 2>&1 1>NUL)
  ifneq ($(filter x86,$(machine_str)),)
    machine := x86
  else ifneq ($(filter x64,$(machine_str)),)
    machine := amd64
  else ifneq ($(filter ARM64,$(machine_str)),)
    machine := arm64
  endif
else
  machine_opt := $(if $(findstring clang,$(compiler)),-print-target-triple,-dumpmachine)
  machine_str := $(shell $(compiler) $(machine_opt))
  ifneq ($(filter i386-% i486-% i586-% i686-%,$(machine_str)),)
    machine := x86
  else ifneq ($(filter amd64-% x86_64-%,$(machine_str)),)
    machine := amd64
  else ifneq ($(filter arm64-% aarch64-%,$(machine_str)),)
    machine := arm64
  else ifneq ($(filter arm-% armv7-%,$(machine_str)),)
    machine := arm32
  else ifneq ($(filter powerpc64-% powerpc64le-%,$(machine_str)),)
    machine := ppc64
  else ifneq ($(filter riscv64-%,$(machine_str)),)
    machine := rv64
  else ifneq ($(filter riscv32-%,$(machine_str)),)
    machine := rv32
  endif

  # detect clang with msvc target
  ifeq ($(msvc),)
    ifneq ($(filter %-msvc,$(machine_str)),)
      msvc := true
    endif
  endif
endif

# global compiler flags
ifeq ($(cl),true)
  flags.c      = -TC -std:c11
  flags.cpp    = -TP -std:c++17 -EHsc
  flags       += -nologo -permissive- -utf-8 -W2 -Fd$(object.path)/
  options     += -nologo $(if $(findstring clang,$(compiler)),-fuse-ld=lld) -link
else
  flags.c      = -x c -std=c11
  flags.cpp    = -x c++ -std=c++17
  flags.objc   = -x objective-c -std=c11
  flags.objcpp = -x objective-c++ -std=c++17
  flags.deps   = -MMD -MP -MF $(@:.o=.d)
endif

# architecture detection
ifeq ($(arch),)
  ifneq ($(machine),)
    arch := $(machine)
  else
    $(error unknown arch, please specify manually.)
  endif
endif

# explicit architecture flags to allow for cross-compilation on macos
ifeq ($(platform),macos)
  ifeq ($(arch),amd64)
    flags += -arch x86_64
    options += -arch x86_64
  else ifeq ($(arch),arm64)
    flags += -arch arm64
    options += -arch arm64
  endif
  ifneq ($(machine),$(arch))
    local = false
  endif
endif

# build optimization levels
ifeq ($(build),debug)
  symbols = true
  lto = false
  ifeq ($(cl),true)
    flags += -Od
  else
    flags += -Og
  endif
  flags += -DBUILD_DEBUG
else ifeq ($(build),stable)
  flags += -O1 -DBUILD_STABLE
else ifeq ($(build),minified)
  flags += -Os -DBUILD_MINIFIED
else ifeq ($(build),release)
  flags += -O2 -DBUILD_RELEASE
else ifeq ($(build),optimized)
  ifeq ($(cl),true)
    flags += -O2
  else
    flags += -O3
  endif
  flags += -DBUILD_OPTIMIZED
else
  $(error unrecognized build type.)
endif

ifeq ($(local),true)
  flags += -DBUILD_LOCAL
endif

# debugging information
ifeq ($(symbols),true)
  ifeq ($(cl),true)
    flags += -Zi -FS
    options += -debug
    ifneq ($(build),debug)
      options += -opt:ref,icf
    endif
  else
    flags += -g
    ifeq ($(platform),windows)
      ifeq ($(findstring clang++,$(compiler)),clang++)
        ifeq ($(symformat),gdb)
          flags += -ggdb
        else  
          flags += -gcodeview
        endif
        ifeq ($(msvc),true)
          options += -Wl,-debug
        else
          options += -Wl,-pdb=
        endif
      endif
    endif
  endif
endif

# link-time optimization
ifeq ($(lto),true)
  ifeq ($(cl),true)
    ifneq ($(findstring clang,$(compiler)),clang)
      flags   += -GL
      options += -ltcg:incremental -ltcgout:$(object.path)/$(name).iobj
    else
      flags   += -flto=thin
      options += -lldltocache:$(object.path)/lto
    endif
  else
    ifneq ($(findstring clang++,$(compiler)),clang++)
      flags   += -flto=auto -fno-fat-lto-objects
    else
      flags   += -flto=thin
      options += -flto=thin
      ifeq ($(platform),macos)
        options += -Wl,-cache_path_lto,$(object.path)/lto
      else ifeq ($(msvc),true)
        options += -Wl,-lldltocache:$(object.path)/lto
      else
        options += -Wl,--thinlto-cache-dir=$(object.path)/lto
      endif
    endif
  endif
endif

# openmp support
ifeq ($(openmp),true)
  # macOS Xcode does not ship with OpenMP support
  ifneq ($(platform),macos)
    flags   += -fopenmp
    options += -fopenmp
  endif
endif

# clang settings
ifeq ($(findstring clang++,$(compiler)),clang++)
  flags += -fno-strict-aliasing -fwrapv
  ifneq ($(platform),macos)
    options += -fuse-ld=lld
  endif
# gcc settings
else ifeq ($(findstring g++,$(compiler)),g++)
  flags += -fno-strict-aliasing -fwrapv -Wno-trigraphs
endif

# windows settings
ifeq ($(platform),windows)
  extension := .exe
  # target Windows 7
  flags += -D_WIN32_WINNT=0x0601
  ifeq ($(msvc),true)
    flags += -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS
  endif
  options += $(call lib,ws2_32 ole32 shell32 shlwapi advapi32)
  options += $(if $(findstring g++,$(compiler)),-mthreads)
  ifeq ($(cl),true)
    flags += $(if $(filter-out true,$(static)),-MD,-MT)
  else ifeq ($(msvc),true)
    flags += -fms-runtime-lib=$(if $(filter-out true,$(static)),dll,static)
  else
    options += $(if $(filter-out true,$(static)),,-static)
  endif
  ifeq ($(console),true)
    flags += -DSUBSYTEM_CONSOLE
    options += $(if $(findstring g++,$(compiler)),-mconsole)
  else
    flags += -DSUBSYTEM_WINDOWS
    options += $(if $(findstring g++,$(compiler)),-mwindows)
  endif
  ifeq ($(windres),)
    ifeq ($(msvc),true)
      windres := $(if $(findstring clang,$(compiler)),llvm-rc,rc)
    else
      windres := windres
    endif
  endif
  ifneq ($(filter cl,$(compiler)),)
    ifeq ($(arch),arm64)
      # work around optimizer bug in MSVC 19.39+
      # https://developercommunity.microsoft.com/t/ARM64-code-generation-regression/10686742#T-N10699503
      flags += -d2lowered-bit-liveness-
    endif
  endif
endif

# macos settings
ifeq ($(platform),macos)
  flags   += -mmacosx-version-min=10.9 -Wno-auto-var-id -Wno-deprecated-declarations -fobjc-arc
  options += -lc++ -lobjc -mmacosx-version-min=10.9
  # allow mprotect() on dynamic recompiler code blocks
  options += -Wl,-segprot,__DATA,rwx,rw
endif

# linux settings
ifeq ($(platform),linux)
  options += -ldl
endif

# bsd settings
ifeq ($(platform),bsd)
  flags   += -I/usr/local/include
  options += -Wl,-rpath=/usr/local/lib
  options += -lstdc++ -lm
endif

# threading support
ifeq ($(threaded),true)
  ifneq ($(filter $(platform),linux bsd),)
    flags   += -pthread
    options += -pthread -lrt
  endif
endif

# pkg-config/pkgconf detection
PKG_CONFIG ?= pkg-config
ifeq ($(shell echo ^^),^)
  PKG_CONFIG_VERSION := $(shell $(PKG_CONFIG) --version 2> NUL)
else
  PKG_CONFIG_VERSION := $(shell $(PKG_CONFIG) --version 2> /dev/null)
endif

pkg_config :=
ifneq ($(PKG_CONFIG_VERSION),)
  pkg_config := $(PKG_CONFIG)
  ifeq ($(cl),true)
    pkg_config += --msvc-syntax
  endif
endif

# paths
ifeq ($(object.path),)
  object.path := obj
endif

ifeq ($(output.path),)
  output.path := out
endif

# rules
default: all;

nall.verbose:
	$(info Compiler:)
	$(info $([space]) $(compiler))
	$(info Compiler Flags:)
	$(foreach n,$(sort $(call unique,$(flags))),$(if $(filter-out -I%,$n),$(info $([space]) $n)))
	$(info Linker Options:)
	$(foreach n,$(sort $(call unique,$(options))),$(if $(filter-out -l%,$n),$(info $([space]) $n)))

%.o: $<
	$(info Compiling $(subst ../,,$<) ...)
	$(call mkdir,$(dir $@))
	@$(call compile)

$(object.path):
	$(call mkdir,$(object.path))

$(output.path):
	$(call mkdir,$(output.path))

# function compile([arguments])
compile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(compiler.c)   $(flags.deps) $(flags) $1 -c $< $(call obj,$@) \
   ,$(if $(filter %.cpp,$<), \
      $(compiler.cpp) $(flags.deps) $(flags) $1 -c $< $(call obj,$@) \
    )) \
  )

# function exe(name)
# function obj(name)
# function lib([names])
ifeq ($(cl),true)
  exe = -Fe$1
  obj = -Fo$1
  lib = $(foreach s,$1,$s.lib)
else
  exe = -o $1
  obj = -o $1
  lib = $(foreach s,$1,-l$s)
endif

# function rwildcard(directory, pattern)
rwildcard = \
  $(strip \
    $(filter $(if $2,$2,%), \
      $(foreach f, \
        $(wildcard $1*), \
        $(eval t = $(call rwildcard,$f/)) \
        $(if $t,$t,$f) \
      ) \
    ) \
  )

# function unique(source)
unique = \
  $(eval __temp :=) \
  $(strip \
    $(foreach s,$1,$(if $(filter $s,$(__temp)),,$(eval __temp += $s))) \
    $(__temp) \
  )

# function strtr(source, from, to)
strtr = \
  $(eval __temp := $1) \
  $(strip \
    $(foreach c, \
      $(join $(addsuffix :,$2),$3), \
      $(eval __temp := \
        $(subst $(word 1,$(subst :, ,$c)),$(word 2,$(subst :, ,$c)),$(__temp)) \
      ) \
    ) \
    $(__temp) \
  )

# function strupper(source)
strupper = $(call strtr,$1,$([a-z]),$([A-Z]))

# function strlower(source)
strlower = $(call strtr,$1,$([A-Z]),$([a-z]))

# function strlen(source)
strlen = \
  $(eval __temp := $(subst $([space]),_,$1)) \
  $(words \
    $(strip \
      $(foreach c, \
        $([all]), \
        $(eval __temp := \
          $(subst $c,$c ,$(__temp)) \
        ) \
      ) \
      $(__temp) \
    ) \
  )

# function streq(source)
streq = $(if $(filter-out xx,x$(subst $1,,$2)$(subst $2,,$1)x),,1)

# function strne(source)
strne = $(if $(filter-out xx,x$(subst $1,,$2)$(subst $2,,$1)x),1,)

# prefix
ifeq ($(platform),windows)
  prefix := $(subst $([space]),\$([space]),$(strip $(call strtr,$(LOCALAPPDATA),\,/)))
else
  prefix := $(HOME)/.local
  flags += -DARES_PREFIX="$(prefix)"
endif

# objects
nall.objects := nall nall-main
nall.objects := $(nall.objects:%=$(object.path)/%.o)

$(object.path)/nall.o: $(nall.path)/nall.cpp
$(object.path)/nall-main.o: $(nall.path)/main.cpp
