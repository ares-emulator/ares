use std::sync::mpsc::Receiver;
use std::sync::Arc;

use glfw::{fail_on_errors, Context, Glfw, GlfwReceiver, PWindow, Window, WindowEvent};

use glow::HasContext;
use librashader_common::{GetSize, Size, Viewport};

use librashader_runtime_gl::{FilterChainGL, GLImage};

const WIDTH: u32 = 800;
const HEIGHT: u32 = 600;
const TITLE: &str = "librashader OpenGL 3.3";

pub fn setup() -> (
    Glfw,
    PWindow,
    GlfwReceiver<(f64, WindowEvent)>,
    glow::Program,
    glow::VertexArray,
    Arc<glow::Context>,
) {
    let mut glfw = glfw::init(fail_on_errors!()).unwrap();
    glfw.window_hint(glfw::WindowHint::ContextVersion(3, 3));
    glfw.window_hint(glfw::WindowHint::OpenGlProfile(
        glfw::OpenGlProfileHint::Core,
    ));
    glfw.window_hint(glfw::WindowHint::OpenGlForwardCompat(true));
    glfw.window_hint(glfw::WindowHint::Resizable(true));
    glfw.window_hint(glfw::WindowHint::OpenGlDebugContext(true));

    let (mut window, events) = glfw
        .create_window(WIDTH, HEIGHT, TITLE, glfw::WindowMode::Windowed)
        .unwrap();
    let (screen_width, screen_height) = window.get_framebuffer_size();

    window.make_current();
    window.set_key_polling(true);
    let mut gl = unsafe { glow::Context::from_loader_function(|ptr| window.get_proc_address(ptr)) };

    unsafe {
        gl.enable(glow::DEBUG_OUTPUT);
        gl.enable(glow::DEBUG_OUTPUT_SYNCHRONOUS);

        gl.debug_message_callback(super::debug_callback);

        gl.debug_message_control(glow::DONT_CARE, glow::DONT_CARE, glow::DONT_CARE, &[], true);
    }

    unsafe {
        gl.viewport(0, 0, screen_width, screen_height);
        gl.clear_color(0.4, 0.4, 0.4, 1.0);
    }
    // -------------------------------------------

    const VERT_SHADER: &str = "#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Color;

out VS_OUTPUT {
    vec3 Color;
} OUT;

void main()
{
    gl_Position = vec4(Position, 1.0);
    OUT.Color = Color;
}";

    const FRAG_SHADER: &str = "#version 330 core

in VS_OUTPUT {
    vec3 Color;
} IN;

layout(location = 0) out vec4 Color;

void main()
{
    Color = vec4(IN.Color, 1.0f);
}";
    let shader_program = super::compile_program(&gl, VERT_SHADER, FRAG_SHADER);

    // unsafe {
    //     glow::ObjectLabel(
    //         glow::SHADER,
    //         shader_program,
    //         -1,
    //         b"color_shader\0".as_ptr().cast(),
    //     );
    // }

    let vertices = &[
        // positions      // colors
        0.5f32, -0.5, 0.0, 1.0, 0.0, 0.0, // bottom right
        -0.5, -0.5, 0.0, 0.0, 1.0, 0.0, // bottom left
        0.0, 0.5, 0.0, 0.0, 0.0, 1.0, // top
    ];
    let vbo;
    unsafe {
        vbo = gl.create_buffer().unwrap();
        // glow::ObjectLabel(glow::BUFFER, vbo, -1, b"triangle_vbo\0".as_ptr().cast());
    }

    unsafe {
        gl.bind_buffer(glow::ARRAY_BUFFER, Some(vbo));
        gl.buffer_data_u8_slice(
            glow::ARRAY_BUFFER, // target
            bytemuck::cast_slice(vertices),
            glow::STATIC_DRAW, // usage
        );
        gl.bind_buffer(glow::ARRAY_BUFFER, None);
    }

    // set up vertex array object

    let vao;
    unsafe {
        vao = gl.create_vertex_array().unwrap();
        // glow::ObjectLabel(glow::VERTEX_ARRAY, vao, -1, b"triangle_vao\0".as_ptr().cast());
    }

    unsafe {
        gl.bind_vertex_array(Some(vao));
        gl.bind_buffer(glow::ARRAY_BUFFER, Some(vbo));

        gl.enable_vertex_attrib_array(0); // this is "layout (location = 0)" in vertex shader
        gl.vertex_attrib_pointer_f32(
            0,           // index of the generic vertex attribute ("layout (location = 0)")
            3,           // the number of components per generic vertex attribute
            glow::FLOAT, // data type
            false,       // normalized (int-to-float conversion)
            (6 * std::mem::size_of::<f32>()) as i32, // stride (byte offset between consecutive attributes)
            0,                                       // offset of the first component
        );
        gl.enable_vertex_attrib_array(1);

        gl.vertex_attrib_pointer_f32(
            1,           // index of the generic vertex attribute ("layout (location = 0)")
            3,           // the number of components per generic vertex attribute
            glow::FLOAT, // data type
            false,       // normalized (int-to-float conversion)
            (6 * std::mem::size_of::<f32>()) as i32, // stride (byte offset between consecutive attributes)
            (3 * std::mem::size_of::<f32>()) as i32, // offset of the first component
        );

        gl.bind_buffer(glow::ARRAY_BUFFER, None);
        gl.bind_vertex_array(None);
    }

    // set up shared state for window

    unsafe {
        gl.viewport(0, 0, 900, 700);
        gl.clear_color(0.3, 0.3, 0.5, 1.0);
    }

    unsafe {
        // -------------------------------------------
        println!("OpenGL version: {}", gl.get_parameter_string(glow::VERSION));
        println!(
            "GLSL version: {}",
            gl.get_parameter_string(glow::SHADING_LANGUAGE_VERSION)
        );
    }

    (glfw, window, events, shader_program, vao, Arc::new(gl))
}

pub fn do_loop(
    gl: &Arc<glow::Context>,
    mut glfw: Glfw,
    mut window: PWindow,
    events: GlfwReceiver<(f64, WindowEvent)>,
    triangle_program: glow::Program,
    triangle_vao: glow::VertexArray,
    filter: &mut FilterChainGL,
) {
    let mut framecount = 0;
    let rendered_framebuffer;
    let rendered_texture;
    let quad_vbuf;

    let output_texture;
    // let output_framebuffer_handle;
    let output_quad_vbuf;

    unsafe {
        // do frmaebuffer
        rendered_framebuffer = gl.create_framebuffer().unwrap();

        gl.bind_framebuffer(glow::FRAMEBUFFER, Some(rendered_framebuffer));

        // glow::ObjectLabel(
        //     glow::FRAMEBUFFER,
        //     rendered_framebuffer,
        //     -1,
        //     b"rendered_framebuffer\0".as_ptr().cast(),
        // );

        // make tetxure
        rendered_texture = gl.create_texture().unwrap();
        gl.bind_texture(glow::TEXTURE_2D, Some(rendered_texture));

        // glow::ObjectLabel(
        //     glow::TEXTURE,
        //     rendered_texture,
        //     -1,
        //     b"rendered_texture\0".as_ptr().cast(),
        // );

        // empty image
        gl.tex_storage_2d(
            glow::TEXTURE_2D,
            1,
            glow::RGBA8,
            WIDTH as i32,
            HEIGHT as i32,
        );

        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_MAG_FILTER,
            glow::NEAREST as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_MIN_FILTER,
            glow::NEAREST as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_WRAP_S,
            glow::CLAMP_TO_EDGE as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_WRAP_T,
            glow::CLAMP_TO_EDGE as i32,
        );

        // set color attachment
        gl.framebuffer_texture_2d(
            glow::FRAMEBUFFER,
            glow::COLOR_ATTACHMENT0,
            glow::TEXTURE_2D,
            Some(rendered_texture),
            0,
        );

        gl.draw_buffer(glow::COLOR_ATTACHMENT0);

        if gl.check_framebuffer_status(glow::FRAMEBUFFER) != glow::FRAMEBUFFER_COMPLETE {
            panic!("failed to create fbo")
        }

        let fullscreen_fbo = [
            -1.0f32, -1.0, 0.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 1.0, -1.0, 0.0,
            1.0, 1.0, 0.0,
        ];

        quad_vbuf = gl.create_buffer().unwrap();
        gl.bind_buffer(glow::ARRAY_BUFFER, Some(quad_vbuf));
        gl.buffer_data_u8_slice(
            glow::ARRAY_BUFFER,
            bytemuck::cast_slice(&fullscreen_fbo),
            glow::STATIC_DRAW,
        );
    }

    unsafe {
        gl.bind_framebuffer(glow::FRAMEBUFFER, None);
        // do frmaebuffer
        // output_framebuffer_handle = gl.create_framebuffer().unwrap();
        //
        // gl.bind_framebuffer(glow::FRAMEBUFFER, Some(output_framebuffer_handle));

        // glow::ObjectLabel(
        //     glow::FRAMEBUFFER,
        //     output_framebuffer_handle,
        //     -1,
        //     b"output_framebuffer\0".as_ptr().cast(),
        // );

        // make tetxure
        output_texture = gl.create_texture().unwrap();
        gl.bind_texture(glow::TEXTURE_2D, Some(output_texture));

        // glow::ObjectLabel(
        //     glow::TEXTURE,
        //     output_texture,
        //     -1,
        //     b"output_texture\0".as_ptr().cast(),
        // );

        // empty image
        gl.tex_storage_2d(
            glow::TEXTURE_2D,
            1,
            glow::RGBA8,
            WIDTH as i32,
            HEIGHT as i32,
        );

        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_MAG_FILTER,
            glow::NEAREST as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_MIN_FILTER,
            glow::NEAREST as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_WRAP_S,
            glow::CLAMP_TO_EDGE as i32,
        );
        gl.tex_parameter_i32(
            glow::TEXTURE_2D,
            glow::TEXTURE_WRAP_T,
            glow::CLAMP_TO_EDGE as i32,
        );

        // set color attachment
        // gl.framebuffer_texture_2d(
        //     glow::FRAMEBUFFER,
        //     glow::COLOR_ATTACHMENT0,
        //     glow::TEXTURE_2D,
        //     Some(output_texture),
        //     0,
        // );

        // gl.draw_buffer(glow::COLOR_ATTACHMENT0);
        // if gl.check_framebuffer_status(glow::FRAMEBUFFER) != glow::FRAMEBUFFER_COMPLETE {
        //     panic!("failed to create fbo")
        // }

        let fullscreen_fbo = [
            -1.0f32, -1.0, 0.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 1.0, -1.0, 0.0,
            1.0, 1.0, 0.0,
        ];

        output_quad_vbuf = gl.create_buffer().unwrap();
        gl.bind_buffer(glow::ARRAY_BUFFER, Some(output_quad_vbuf));
        gl.buffer_data_u8_slice(
            glow::ARRAY_BUFFER, // target
            bytemuck::cast_slice(&fullscreen_fbo),
            glow::STATIC_DRAW, // usage
        );
    }

    const VERT_SHADER: &str = r"#version 150 core
out vec2 v_tex;

const vec2 pos[4]=vec2[4](vec2(-1.0, 1.0),
                          vec2(-1.0,-1.0),
                          vec2( 1.0, 1.0),
                          vec2( 1.0,-1.0));

void main()
{
    v_tex=0.5*pos[gl_VertexID] + vec2(0.5);
    gl_Position=vec4(pos[gl_VertexID], 0.0, 1.0);
}
";

    const FRAG_SHADER: &str = r"
#version 150 core
in vec2 v_tex;
uniform sampler2D texSampler;
out vec4 color;
void main()
{
    color=texture(texSampler, v_tex);
}";

    let quad_programid = super::compile_program(gl, VERT_SHADER, FRAG_SHADER);
    let quad_vao;
    unsafe {
        quad_vao = gl.create_vertex_array().unwrap();
    }

    let (fb_width, fb_height) = window.get_framebuffer_size();
    let (vp_width, vp_height) = window.get_size();

    let output = GLImage {
        handle: Some(output_texture),
        format: glow::RGBA8,
        size: Size::new(vp_width as u32, vp_height as u32),
    };

    while !window.should_close() {
        glfw.poll_events();
        for (_, event) in glfw::flush_messages(&events) {
            glfw_handle_event(&mut window, event);
        }

        unsafe {
            // render to fb
            gl.bind_framebuffer(glow::FRAMEBUFFER, Some(rendered_framebuffer));
            gl.viewport(0, 0, vp_width, vp_height);

            // clear color
            gl.clear_color(0.3, 0.4, 0.6, 1.0);
            gl.clear(glow::COLOR_BUFFER_BIT);

            // do the drawing
            gl.use_program(Some(triangle_program));
            // select vertices
            gl.bind_vertex_array(Some(triangle_vao));

            // draw to bound target
            gl.draw_arrays(glow::TRIANGLES, 0, 3);

            // unselect vertices
            gl.bind_vertex_array(None);

            // unselect fbo
            gl.bind_framebuffer(glow::FRAMEBUFFER, None);
        }

        let viewport = Viewport {
            x: 0f32,
            y: 0f32,
            output: &output,
            mvp: None,
            size: output.size().unwrap(),
        };

        let rendered = GLImage {
            handle: Some(rendered_texture),
            format: glow::RGBA8,
            size: Size {
                width: fb_width as u32,
                height: fb_height as u32,
            },
        };

        unsafe {
            filter
                .frame(&rendered, &viewport, framecount, None)
                .unwrap();
        }

        unsafe {
            // texture is done now.
            // draw quad to screen
            gl.use_program(Some(quad_programid));

            gl.active_texture(glow::TEXTURE0);
            gl.bind_texture(glow::TEXTURE_2D, Some(output_texture));

            gl.bind_vertex_array(Some(quad_vao));
            gl.draw_arrays(glow::TRIANGLE_STRIP, 0, 4);
        }

        framecount += 1;
        window.swap_buffers();
    }
}

fn glfw_handle_event(window: &mut glfw::Window, event: glfw::WindowEvent) {
    use glfw::Action;
    use glfw::Key;
    use glfw::WindowEvent as Event;

    match event {
        Event::Key(Key::Escape, _, Action::Press, _) => {
            window.set_should_close(true);
        }
        Event::Size(width, height) => window.set_size(width, height),
        _ => {}
    }
}
