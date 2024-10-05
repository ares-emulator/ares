mod hello_triangle;

use librashader_runtime_gl::options::FilterChainOptionsGL;
use librashader_runtime_gl::FilterChainGL;
use std::sync::Arc;

#[test]
fn triangle_gl() {
    let (glfw, window, events, shader, vao, context) = hello_triangle::gl3::setup();

    unsafe {
        let mut filter = FilterChainGL::load_from_path(
            // "../test/basic.slangp",
            "../test/shaders_slang/crt/crt-royale.slangp",
            Arc::clone(&context),
            Some(&FilterChainOptionsGL {
                glsl_version: 0,
                use_dsa: false,
                force_no_mipmaps: false,
                disable_cache: true,
            }),
        )
        // FilterChain::load_from_path("../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp", None)
        .expect("Failed to load filter chain");
        hello_triangle::gl3::do_loop(&context, glfw, window, events, shader, vao, &mut filter);
    }
}

#[test]
fn triangle_gl46() {
    let (glfw, window, events, shader, vao, context) = hello_triangle::gl46::setup();
    unsafe {
        let mut filter = FilterChainGL::load_from_path(
            // "../test/slang-shaders/vhs/VHSPro.slangp",
            // "../test/slang-shaders/test/history.slangp",
            // "../test/basic.slangp",
            // "../test/shaders_slang/crt/crt-royale.slangp",
            "../test/shaders_slang/crt/crt-royale.slangp",
            Arc::clone(&context),
            Some(&FilterChainOptionsGL {
                glsl_version: 330,
                use_dsa: true,
                force_no_mipmaps: false,
                disable_cache: false,
            }),
        )
        // FilterChain::load_from_path("../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp", None)
        .unwrap();
        hello_triangle::gl46::do_loop(&context, glfw, window, events, shader, vao, &mut filter);
    }
}
