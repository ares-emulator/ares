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

#include "bind.hpp"
#include "shaders.hpp"
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
  auto allocate() -> void;
  auto size(u32 width, u32 height) -> void;
  auto release() -> void;
  auto render(u32 sourceWidth, u32 sourceHeight, u32 targetX, u32 targetY, u32 targetWidth, u32 targetHeight) -> void;

  GLuint program = 0;
  GLuint framebuffer = 0;
  GLuint vao = 0;
  GLuint vbo[3] = {0, 0, 0};
  GLuint vertex = 0;
  GLuint geometry = 0;
  GLuint fragment = 0;
  u32* buffer = nullptr;
};

struct OpenGLProgram : OpenGLSurface {
  auto bind(OpenGL* instance, const Markup::Node& node, const string& pathname) -> void;
  auto parse(OpenGL* instance, string& source) -> void;
  auto release() -> void;

  u32 phase = 0;   //frame counter
  u32 modulo = 0;  //frame counter modulus
  u32 absoluteWidth = 0;
  u32 absoluteHeight = 0;
  f64 relativeWidth = 0;
  f64 relativeHeight = 0;
  vector<OpenGLTexture> pixmaps;
};

struct OpenGL : OpenGLProgram {
  auto setShader(const string& pathname) -> void;
  auto allocateHistory(u32 size) -> void;
  auto clear() -> void;
  auto lock(u32*& data, u32& pitch) -> bool;
  auto output() -> void;
  auto initialize(const string& shader) -> bool;
  auto terminate() -> void;

  vector<OpenGLProgram> programs;
  vector<OpenGLTexture> history;
  GLuint inputFormat = GL_RGBA8;
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
  set<Setting> settings;
  bool initialized = false;
};

#include "texture.hpp"
#include "surface.hpp"
#include "program.hpp"
#include "main.hpp"
