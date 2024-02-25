mod hello_triangle;

use librashader_runtime_gl::options::FilterChainOptionsGL;
use librashader_runtime_gl::FilterChainGL;

#[test]
fn triangle_gl() {
    let (glfw, window, events, shader, vao) = hello_triangle::gl3::setup();

    unsafe {
        let mut filter = FilterChainGL::load_from_path(
            "../test/shaders_slang/crt/crt-royale.slangp",
            Some(&FilterChainOptionsGL {
                glsl_version: 0,
                use_dsa: false,
                force_no_mipmaps: false,
                disable_cache: false,
            }),
        )
        // FilterChain::load_from_path("../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp", None)
        .unwrap();
        hello_triangle::gl3::do_loop(glfw, window, events, shader, vao, &mut filter);
    }
}

#[test]
fn triangle_gl46() {
    let (glfw, window, events, shader, vao) = hello_triangle::gl46::setup();
    unsafe {
        let mut filter = FilterChainGL::load_from_path(
            // "../test/slang-shaders/vhs/VHSPro.slangp",
            // "../test/slang-shaders/test/history.slangp",
            "../test/shaders_slang/crt/crt-royale.slangp",
            // "../test/shadersslang/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
            Some(&FilterChainOptionsGL {
                glsl_version: 0,
                use_dsa: true,
                force_no_mipmaps: false,
                disable_cache: false,
            }),
        )
        // FilterChain::load_from_path("../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp", None)
        .unwrap();
        hello_triangle::gl46::do_loop(glfw, window, events, shader, vao, &mut filter);
    }
}
