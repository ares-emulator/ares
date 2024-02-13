use std::convert::TryInto;
use std::ffi::{c_void, CStr};
use std::sync::mpsc::Receiver;

use glfw::{Context, Glfw, Window, WindowEvent};

use gl::types::{GLchar, GLenum, GLint, GLsizei, GLuint};
use librashader_common::{Size, Viewport};

use librashader_runtime_gl::{FilterChainGL, GLFramebuffer, GLImage};

const WIDTH: u32 = 800;
const HEIGHT: u32 = 600;
const TITLE: &str = "librashader OpenGL 4.6";

pub fn compile_program(vertex: &str, fragment: &str) -> GLuint {
    let vertex_shader = unsafe { gl::CreateShader(gl::VERTEX_SHADER) };
    unsafe {
        gl::ShaderSource(
            vertex_shader,
            1,
            &vertex.as_bytes().as_ptr().cast(),
            &vertex.len().try_into().unwrap(),
        );
        gl::CompileShader(vertex_shader);

        let mut success = 0;
        gl::GetShaderiv(vertex_shader, gl::COMPILE_STATUS, &mut success);
        if success == 0 {
            let mut log_len = 0_i32;
            // gl::GetShaderiv(vertex_shader, gl::INFO_LOG_LENGTH, &mut log_len);
            // let mut v: Vec<u8> = Vec::with_capacity(log_len as usize);
            // gl::GetShaderInfoLog(vertex_shader, log_len, &mut log_len, v.as_mut_ptr().cast());
            let mut v: Vec<u8> = Vec::with_capacity(1024);
            gl::GetShaderInfoLog(vertex_shader, 1024, &mut log_len, v.as_mut_ptr().cast());
            v.set_len(log_len.try_into().unwrap());
            panic!(
                "Vertex Shader Compile Error: {}",
                String::from_utf8_lossy(&v)
            );
        }
    }

    let fragment_shader = unsafe { gl::CreateShader(gl::FRAGMENT_SHADER) };
    unsafe {
        gl::ShaderSource(
            fragment_shader,
            1,
            &fragment.as_bytes().as_ptr().cast(),
            &fragment.len().try_into().unwrap(),
        );
        gl::CompileShader(fragment_shader);

        let mut success = 0;
        gl::GetShaderiv(fragment_shader, gl::COMPILE_STATUS, &mut success);
        if success == 0 {
            let mut v: Vec<u8> = Vec::with_capacity(1024);
            let mut log_len = 0_i32;
            gl::GetShaderInfoLog(fragment_shader, 1024, &mut log_len, v.as_mut_ptr().cast());
            v.set_len(log_len.try_into().unwrap());
            panic!(
                "Fragment Shader Compile Error: {}",
                String::from_utf8_lossy(&v)
            );
        }
    }

    let shader_program = unsafe { gl::CreateProgram() };
    unsafe {
        gl::AttachShader(shader_program, vertex_shader);
        gl::AttachShader(shader_program, fragment_shader);
        gl::LinkProgram(shader_program);

        let mut success = 0;
        gl::GetProgramiv(shader_program, gl::LINK_STATUS, &mut success);
        if success == 0 {
            let mut v: Vec<u8> = Vec::with_capacity(1024);
            let mut log_len = 0_i32;
            gl::GetProgramInfoLog(shader_program, 1024, &mut log_len, v.as_mut_ptr().cast());
            v.set_len(log_len.try_into().unwrap());
            panic!("Program Link Error: {}", String::from_utf8_lossy(&v));
        }

        gl::DetachShader(shader_program, vertex_shader);
        gl::DetachShader(shader_program, fragment_shader);
        gl::DeleteShader(vertex_shader);
        gl::DeleteShader(fragment_shader);
    }

    shader_program
}

extern "system" fn debug_callback(
    _source: GLenum,
    _err_type: GLenum,
    _id: GLuint,
    _severity: GLenum,
    _length: GLsizei,
    message: *const GLchar,
    _user: *mut c_void,
) {
    unsafe {
        let message = CStr::from_ptr(message);
        println!("[gl] {message:?}");
    }
}

pub fn setup() -> (Glfw, Window, Receiver<(f64, WindowEvent)>, GLuint, GLuint) {
    let mut glfw = glfw::init(glfw::FAIL_ON_ERRORS).unwrap();
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
    gl::load_with(|ptr| window.get_proc_address(ptr) as *const _);

    unsafe {
        gl::Enable(gl::DEBUG_OUTPUT);
        gl::Enable(gl::DEBUG_OUTPUT_SYNCHRONOUS);

        gl::DebugMessageCallback(Some(debug_callback), std::ptr::null_mut());
        gl::DebugMessageControl(
            gl::DONT_CARE,
            gl::DONT_CARE,
            gl::DONT_CARE,
            0,
            std::ptr::null(),
            gl::TRUE,
        );
    }

    unsafe {
        gl::Viewport(0, 0, screen_width, screen_height);
        clear_color(Color(0.4, 0.4, 0.4, 1.0));
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
    let shader_program = compile_program(VERT_SHADER, FRAG_SHADER);

    unsafe {
        gl::ObjectLabel(
            gl::SHADER,
            shader_program,
            -1,
            b"color_shader\0".as_ptr().cast(),
        );
    }

    let vertices = &[
        // positions      // colors
        0.5f32, -0.5, 0.0, 1.0, 0.0, 0.0, // bottom right
        -0.5, -0.5, 0.0, 0.0, 1.0, 0.0, // bottom left
        0.0, 0.5, 0.0, 0.0, 0.0, 1.0, // top
    ];
    let mut vbo: gl::types::GLuint = 0;
    unsafe {
        gl::CreateBuffers(1, &mut vbo);
        gl::ObjectLabel(gl::BUFFER, vbo, -1, b"triangle_vbo\0".as_ptr().cast());
    }

    unsafe {
        gl::NamedBufferData(
            vbo,
            (vertices.len() * std::mem::size_of::<f32>()) as gl::types::GLsizeiptr, // size of data in bytes
            vertices.as_ptr() as *const gl::types::GLvoid, // pointer to data
            gl::STATIC_DRAW,                               // usage
        );
    }

    // set up vertex array object

    let mut vao: gl::types::GLuint = 0;

    // todo: figure this shit out
    unsafe {
        gl::CreateVertexArrays(1, &mut vao);
        gl::ObjectLabel(gl::VERTEX_ARRAY, vao, -1, b"triangle_vao\0".as_ptr().cast());

        gl::VertexArrayVertexBuffer(vao, 0, vbo, 0, 6 * std::mem::size_of::<f32>() as GLint);

        gl::EnableVertexArrayAttrib(vao, 0); // this is "layout (location = 0)" in vertex shader
        gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE, 0);

        gl::EnableVertexArrayAttrib(vao, 1);
        gl::VertexArrayAttribFormat(
            vao,
            1,
            3,
            gl::FLOAT,
            gl::FALSE,
            3 * std::mem::size_of::<f32>() as GLuint,
        );

        gl::VertexArrayAttribBinding(vao, 0, 0);
        gl::VertexArrayAttribBinding(vao, 1, 0);
    }

    // set up shared state for window

    unsafe {
        gl::Viewport(0, 0, 900, 700);
        gl::ClearColor(0.3, 0.3, 0.5, 1.0);
    }

    // -------------------------------------------
    println!("OpenGL version: {}", gl_get_string(gl::VERSION));
    println!(
        "GLSL version: {}",
        gl_get_string(gl::SHADING_LANGUAGE_VERSION)
    );

    (glfw, window, events, shader_program, vao)
}

pub fn do_loop(
    mut glfw: Glfw,
    mut window: Window,
    events: Receiver<(f64, WindowEvent)>,
    triangle_program: GLuint,
    triangle_vao: GLuint,
    filter: &mut FilterChainGL,
) {
    let mut framecount = 0;
    let mut rendered_framebuffer = 0;
    let mut rendered_texture = 0;
    let mut quad_vbuf = 0;

    let mut output_texture = 0;
    let mut output_framebuffer_handle = 0;
    let mut output_quad_vbuf = 0;

    unsafe {
        // do frmaebuffer
        gl::CreateFramebuffers(1, &mut rendered_framebuffer);

        gl::ObjectLabel(
            gl::FRAMEBUFFER,
            rendered_framebuffer,
            -1,
            b"rendered_framebuffer\0".as_ptr().cast(),
        );

        // make tetxure
        gl::CreateTextures(gl::TEXTURE_2D, 1, &mut rendered_texture);

        gl::ObjectLabel(
            gl::TEXTURE,
            rendered_texture,
            -1,
            b"rendered_texture\0".as_ptr().cast(),
        );

        // empty image
        gl::TextureStorage2D(
            rendered_texture,
            1,
            gl::RGBA8,
            WIDTH as GLsizei,
            HEIGHT as GLsizei,
        );

        gl::TextureParameteri(
            rendered_texture,
            gl::TEXTURE_MAG_FILTER,
            gl::NEAREST as GLint,
        );
        gl::TextureParameteri(
            rendered_texture,
            gl::TEXTURE_MIN_FILTER,
            gl::NEAREST as GLint,
        );
        gl::TextureParameteri(
            rendered_texture,
            gl::TEXTURE_WRAP_S,
            gl::CLAMP_TO_EDGE as GLint,
        );
        gl::TextureParameteri(
            rendered_texture,
            gl::TEXTURE_WRAP_T,
            gl::CLAMP_TO_EDGE as GLint,
        );

        // set color attachment
        gl::NamedFramebufferTexture(
            rendered_framebuffer,
            gl::COLOR_ATTACHMENT0,
            rendered_texture,
            0,
        );

        let buffers = [gl::COLOR_ATTACHMENT0];
        gl::NamedFramebufferDrawBuffers(rendered_framebuffer, 1, buffers.as_ptr());

        if gl::CheckFramebufferStatus(gl::FRAMEBUFFER) != gl::FRAMEBUFFER_COMPLETE {
            panic!("failed to create fbo")
        }

        let fullscreen_fbo = [
            -1.0f32, -1.0, 0.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 1.0, -1.0, 0.0,
            1.0, 1.0, 0.0,
        ];

        gl::CreateBuffers(1, &mut quad_vbuf);
        gl::NamedBufferData(
            quad_vbuf,                                                                    // target
            (fullscreen_fbo.len() * std::mem::size_of::<f32>()) as gl::types::GLsizeiptr, // size of data in bytes
            fullscreen_fbo.as_ptr() as *const gl::types::GLvoid, // pointer to data
            gl::STATIC_DRAW,                                     // usage
        );
    }

    unsafe {
        // do frmaebuffer
        gl::CreateFramebuffers(1, &mut output_framebuffer_handle);

        gl::ObjectLabel(
            gl::FRAMEBUFFER,
            output_framebuffer_handle,
            -1,
            b"output_framebuffer\0".as_ptr().cast(),
        );

        // make tetxure
        gl::CreateTextures(gl::TEXTURE_2D, 1, &mut output_texture);

        gl::ObjectLabel(
            gl::TEXTURE,
            output_texture,
            -1,
            b"output_texture\0".as_ptr().cast(),
        );

        // empty image
        gl::TextureStorage2D(
            output_texture,
            1,
            gl::RGBA8,
            WIDTH as GLsizei,
            HEIGHT as GLsizei,
        );

        gl::TextureParameteri(output_texture, gl::TEXTURE_MAG_FILTER, gl::NEAREST as GLint);
        gl::TextureParameteri(output_texture, gl::TEXTURE_MIN_FILTER, gl::NEAREST as GLint);
        gl::TextureParameteri(
            output_texture,
            gl::TEXTURE_WRAP_S,
            gl::CLAMP_TO_EDGE as GLint,
        );
        gl::TextureParameteri(
            output_texture,
            gl::TEXTURE_WRAP_T,
            gl::CLAMP_TO_EDGE as GLint,
        );

        // set color attachment
        gl::NamedFramebufferTexture(
            output_framebuffer_handle,
            gl::COLOR_ATTACHMENT0,
            output_texture,
            0,
        );

        let buffers = [gl::COLOR_ATTACHMENT0];
        gl::NamedFramebufferDrawBuffers(output_framebuffer_handle, 1, buffers.as_ptr());

        if gl::CheckFramebufferStatus(gl::FRAMEBUFFER) != gl::FRAMEBUFFER_COMPLETE {
            panic!("failed to create fbo")
        }

        let fullscreen_fbo = [
            -1.0f32, -1.0, 0.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 1.0, -1.0, 0.0,
            1.0, 1.0, 0.0,
        ];

        gl::CreateBuffers(1, &mut output_quad_vbuf);
        gl::NamedBufferData(
            output_quad_vbuf,
            (fullscreen_fbo.len() * std::mem::size_of::<f32>()) as gl::types::GLsizeiptr, // size of data in bytes
            fullscreen_fbo.as_ptr() as *const gl::types::GLvoid, // pointer to data
            gl::STATIC_DRAW,                                     // usage
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

    let quad_programid = compile_program(VERT_SHADER, FRAG_SHADER);
    let mut quad_vao = 0;
    unsafe {
        gl::CreateVertexArrays(1, &mut quad_vao);
    }

    let (fb_width, fb_height) = window.get_framebuffer_size();
    let (vp_width, vp_height) = window.get_size();

    let output = GLFramebuffer::new_from_raw(
        output_texture,
        output_framebuffer_handle,
        gl::RGBA8,
        Size::new(vp_width as u32, vp_height as u32),
        1,
    );

    while !window.should_close() {
        glfw.poll_events();
        for (_, event) in glfw::flush_messages(&events) {
            glfw_handle_event(&mut window, event);
        }

        unsafe {
            // render to fb

            gl::ClearNamedFramebufferfv(
                rendered_framebuffer,
                gl::COLOR,
                0,
                [0.3f32, 0.4, 0.6, 1.0].as_ptr().cast(),
            );
            gl::BindFramebuffer(gl::FRAMEBUFFER, rendered_framebuffer);
            gl::Viewport(0, 0, vp_width, vp_height);

            // do the drawing
            gl::UseProgram(triangle_program);
            // select vertices
            gl::BindVertexArray(triangle_vao);

            // draw to bound target
            gl::DrawArrays(gl::TRIANGLES, 0, 3);

            // unselect vertices
            gl::BindVertexArray(0);

            // unselect fbo
            gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
        }

        let viewport = Viewport {
            x: 0f32,
            y: 0f32,
            output: &output,
            mvp: None,
        };

        let rendered = GLImage {
            handle: rendered_texture,
            format: gl::RGBA8,
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
            gl::UseProgram(quad_programid);

            gl::BindTextureUnit(0, output_texture);

            gl::BindVertexArray(quad_vao);
            gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
        }

        framecount += 1;
        window.swap_buffers();
    }
}

pub struct Color(f32, f32, f32, f32);

pub fn clear_color(c: Color) {
    unsafe { gl::ClearColor(c.0, c.1, c.2, c.3) }
}

pub fn gl_get_string<'a>(name: gl::types::GLenum) -> &'a str {
    let v = unsafe { gl::GetString(name) };
    let v: &std::ffi::CStr = unsafe { std::ffi::CStr::from_ptr(v as *const i8) };
    v.to_str().unwrap()
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
