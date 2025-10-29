#include <set>

#if defined(DISPLAY_XORG)
  #include <GL/gl.h>
  #include <GL/glx.h>
  #ifndef glGetProcAddress
    #define glGetProcAddress(name) (*glXGetProcAddress)((const GLubyte*)(name))
  #endif
#elif defined(DISPLAY_QUARTZ)
  #include <OpenGL/gl3.h>
#elif defined(DISPLAY_WINDOWS)
  #include <GL/gl.h>
  #include <GL/glext.h>
  #ifndef glGetProcAddress
    #define glGetProcAddress(name) wglGetProcAddress(name)
  #endif
#else
  #error "ruby::OpenGL3: unsupported platform"
#endif

#include <librashader/librashader_ld.h>

#include "bind.hpp"
#include "utility.hpp"

struct OpenGL;

struct OpenGLTexture {
  auto getFormat() const -> GLuint;
  auto getType() const -> GLuint;

  GLuint texture = 0;
  u32 width = 0;
  u32 height = 0;
  GLuint format = GL_RGBA8;
  GLuint filter = GL_LINEAR;
  GLuint wrap = GL_CLAMP_TO_BORDER;
};

struct OpenGLSurface : OpenGLTexture {
  auto size(u32 width, u32 height) -> void;
  auto release() -> void;
  auto render(u32 sourceWidth, u32 sourceHeight, u32 targetX, u32 targetY, u32 targetWidth, u32 targetHeight) -> void;

  GLuint framebuffer = 0;
  GLuint framebufferTexture = 0;
  GLuint framebufferFormat = 0;
  GLuint framebufferWidth = 0;
  GLuint framebufferHeight = 0;
  u32* buffer = nullptr;

  libra_instance_t _libra;
  libra_shader_preset_t _preset = NULL;
  libra_gl_filter_chain_t  _chain = NULL;
  u32 frameCount = 0;
};

struct OpenGL : OpenGLSurface {
  auto setShader(const string& pathname) -> void;
  auto clear() -> void;
  auto lock(u32*& data, u32& pitch) -> bool;
  auto output() -> void;
  auto initialize(const string& shader) -> bool;
  auto terminate() -> void;
  static auto resolveSymbol(const char* name) -> const void*;

  GLuint inputFormat = GL_RGBA8;
  u32 absoluteWidth = 0;
  u32 absoluteHeight = 0;
  u32 outputX = 0;
  u32 outputY = 0;
  u32 outputWidth = 0;
  u32 outputHeight = 0;
  struct Setting {
    string name;
    string value;
    bool operator< (const Setting& source) const { return name <  source.name; }
    bool operator==(const Setting& source) const { return name == source.name; }
    Setting() = default;
    Setting(const string& name) : name(name) {}
    Setting(const string& name, const string& value) : name(name), value(value) {}
  };
  std::set<Setting> settings;
  bool initialized = false;
};

#include "texture.hpp"
#include "surface.hpp"
#include "main.hpp"
