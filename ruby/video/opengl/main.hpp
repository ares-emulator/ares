#if defined(PLATFORM_MACOS)
#import <mach-o/dyld.h>
#endif
auto OpenGL::setShader(const string& pathname) -> void {
  settings.reset();

  format = inputFormat;
  filter = GL_NEAREST;
  wrap = GL_CLAMP_TO_BORDER;
  absoluteWidth = 0, absoluteHeight = 0;

  if(_chain != NULL) {
    _libra.gl_filter_chain_free(&_chain);
  }

  if(_preset != NULL) {
    _libra.preset_free(&_preset);
  }

  if(file::exists(pathname)) {
    if(_libra.preset_create(pathname.data(), &_preset) != NULL) {
      print(string{"OpenGL: Failed to load shader: ", pathname, "\n"});
      setShader("");
      return;
    }

    if(auto error = _libra.gl_filter_chain_create(&_preset, resolveSymbol, NULL, &_chain)) {
      print(string{"OpenGL: Failed to create filter chain for: ", pathname, "\n"});
      _libra.error_print(error);
      setShader("");
      return;
    }
  }
}

auto OpenGL::clear() -> void {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
}

auto OpenGL::lock(u32*& data, u32& pitch) -> bool {
  pitch = width * sizeof(u32);
  return data = buffer;
}

auto OpenGL::output() -> void {
  clear();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, getFormat(), getType(), buffer);
  glGenerateMipmap(GL_TEXTURE_2D);

  struct Source {
    GLuint texture;
    u32 width, height;
    GLuint filter, wrap;
  };
  vector<Source> sources;
  sources.prepend({texture, width, height, filter, wrap});

  u32 targetWidth = absoluteWidth ? absoluteWidth : outputWidth;
  u32 targetHeight = absoluteHeight ? absoluteHeight : outputHeight;

  u32 x = (outputWidth - targetWidth) / 2;
  u32 y = (outputHeight - targetHeight) / 2;

  if(_chain != NULL) {
    // Shader path: our intermediate framebuffer matches the target size (final composited game area size)
    if(!framebuffer || framebufferWidth != targetWidth || framebufferHeight != targetHeight) {
      if(framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        framebuffer = 0;
      }
      if(framebufferTexture) {
        glDeleteTextures(1, &framebufferTexture);
        framebufferTexture = 0;
      }

      framebufferWidth = targetWidth, framebufferHeight = targetHeight;
      glGenFramebuffers(1, &framebuffer);
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
      glGenTextures(1, &framebufferTexture);
      glBindTexture(GL_TEXTURE_2D, framebufferTexture);
      framebufferFormat = GL_RGB;

      glTexImage2D(GL_TEXTURE_2D, 0, framebufferFormat, framebufferWidth, framebufferHeight, 0, framebufferFormat,
                   GL_UNSIGNED_BYTE, nullptr);

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);
    }
  } else {
    // Non-shader path: our intermediate framebuffer matches the source size and re-uses the source texture
    if(!framebuffer || framebufferWidth != width || framebufferHeight != height) {
      if(framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        framebuffer = 0;
      }

      if(framebufferTexture) {
        glDeleteTextures(1, &framebufferTexture);
        framebufferTexture = 0;
      }

      framebufferWidth = width, framebufferHeight = height;
      glGenFramebuffers(1, &framebuffer);
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    }
  }

  render(sources[0].width, sources[0].height, outputX + x, outputY + y, targetWidth, targetHeight);
}

auto OpenGL::initialize(const string& shader) -> bool {
  if(!OpenGLBind()) return false;

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_POLYGON_SMOOTH);
  glDisable(GL_STENCIL_TEST);
  glEnable(GL_DITHER);

  _libra = librashader_load_instance();
  if(!_libra.instance_loaded) {
    print("OpenGL: Failed to load librashader: shaders will be disabled\n");
  }

  setShader(shader);
  return initialized = true;
}

auto OpenGL::resolveSymbol(const char* name) -> const void * {
#if defined(PLATFORM_MACOS)
  NSSymbol symbol;
  char *symbolName;
  symbolName = (char*)malloc(strlen(name) + 2);
  strcpy(symbolName + 1, name);
  symbolName[0] = '_';
  symbol = NULL;
  if(NSIsSymbolNameDefined (symbolName)) symbol = NSLookupAndBindSymbol (symbolName);
  free(symbolName); // 5
  return (void*)(symbol ? NSAddressOfSymbol(symbol) : NULL);
#else
  void* symbol = (void*)glGetProcAddress(name);
  #if defined(PLATFORM_WINDOWS)
    if(!symbol) {
      // (w)glGetProcAddress will not return function pointers from any OpenGL functions
      // that are directly exported by the opengl32.dll
      HMODULE module = LoadLibraryA("opengl32.dll");
      symbol = (void*)GetProcAddress(module, name);
    }
  #endif
#endif

  return symbol;
}

auto OpenGL::terminate() -> void {
  if(!initialized) return;
  setShader("");
  OpenGLSurface::release();
  if(buffer) { delete[] buffer; buffer = nullptr; }
  initialized = false;
}
