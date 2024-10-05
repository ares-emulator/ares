# librashader C headers

The librashader C headers are unlike the implementations, explicitly licensed under the MIT license.

They are provided for easy integration of librashader in a multi-target C or C++ project that may not have
the necessary hardware or APIs available required for all supported runtimes. 

`librashader.h` can be depended upon to link with `librashader.dll` or `librashader.so` if you wish to link 
with librashader. 

An easier alternative is to use the `librashader_ld.h` header library to load function pointers
from any `librashader.dll` or `librashader.so` implementation in the search path. You should customize this
header file to remove support for any runtimes you do not need.

## Usage

A basic example of using `librashader_ld.h` to load a shader preset.

```c++
#include "librashader_ld.h"

libra_gl_filter_chain_t load_gl_filter_chain(libra_gl_loader_t opengl, const char *preset_path) {
  libra_instance_t librashader = librashader_load_instance();
  
  if (!librashader.instance_loaded) {
    std::cout << "Could not load librashader\n";
    return NULL;
  }
  
  libra_shader_preset_t preset;
  libra_error_t error = librashader.preset_create(preset_path, &preset);
  if (error != NULL) {
    std::cout << "Could not load preset\n";
    return NULL;
  }
  
  libra_gl_filter_chain_t chain;
  if (librashader.gl_filter_chain_create(&preset, opengl, NULL, &chain) {
    std::cout << "Could not create OpenGL filter chain\n";
  }
  return chain;
}
```
