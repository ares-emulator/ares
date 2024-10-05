use glow::HasContext;

pub mod gl3;
pub mod gl46;

pub fn compile_program(gl: &glow::Context, vertex: &str, fragment: &str) -> glow::Program {
    let vertex_shader = unsafe { gl.create_shader(glow::VERTEX_SHADER).unwrap() };
    unsafe {
        gl.shader_source(vertex_shader, &vertex);
        gl.compile_shader(vertex_shader);

        if !gl.get_shader_compile_status(vertex_shader) {
            let error = gl.get_shader_info_log(vertex_shader);
            panic!("Vertex Shader Compile Error: {error}",);
        }
    }

    let fragment_shader = unsafe { gl.create_shader(glow::FRAGMENT_SHADER).unwrap() };
    unsafe {
        gl.shader_source(fragment_shader, &fragment);
        gl.compile_shader(fragment_shader);

        if !gl.get_shader_compile_status(fragment_shader) {
            let error = gl.get_shader_info_log(fragment_shader);
            panic!("Fragment Shader Compile Error: {error}",);
        }
    }

    let shader_program = unsafe { gl.create_program().unwrap() };
    unsafe {
        gl.attach_shader(shader_program, vertex_shader);
        gl.attach_shader(shader_program, fragment_shader);
        gl.link_program(shader_program);

        if !gl.get_program_link_status(shader_program) {
            let error = gl.get_program_info_log(shader_program);
            panic!("Program Link Error: {error}",);
        }

        gl.detach_shader(shader_program, vertex_shader);
        gl.detach_shader(shader_program, fragment_shader);
        gl.delete_shader(vertex_shader);
        gl.delete_shader(fragment_shader);
    }

    shader_program
}

fn debug_callback(_source: u32, _err_type: u32, _id: u32, _severity: u32, message: &str) {
    println!("[gl] {message:?}");
}
