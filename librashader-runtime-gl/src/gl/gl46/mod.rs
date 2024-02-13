mod draw_quad;
mod framebuffer;
mod lut_load;
mod texture_bind;
mod ubo_ring;

mod compile_program;

use crate::gl::gl46::compile_program::Gl4CompileProgram;
use crate::gl::GLInterface;
use draw_quad::*;
use framebuffer::*;
use lut_load::*;
use texture_bind::*;
use ubo_ring::*;

pub struct DirectStateAccessGL;
impl GLInterface for DirectStateAccessGL {
    type FramebufferInterface = Gl46Framebuffer;
    type UboRing = Gl46UboRing<16>;
    type DrawQuad = Gl46DrawQuad;
    type LoadLut = Gl46LutLoad;
    type BindTexture = Gl46BindTexture;
    type CompileShader = Gl4CompileProgram;
}
