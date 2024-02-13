mod compile_program;
mod draw_quad;
mod framebuffer;
mod lut_load;
mod texture_bind;
mod ubo_ring;

use crate::gl::GLInterface;
use compile_program::*;
use draw_quad::*;
use framebuffer::*;
use lut_load::*;
use texture_bind::*;
use ubo_ring::*;

pub struct CompatibilityGL;
impl GLInterface for CompatibilityGL {
    type FramebufferInterface = Gl3Framebuffer;
    type UboRing = Gl3UboRing<16>;
    type DrawQuad = Gl3DrawQuad;
    type LoadLut = Gl3LutLoad;
    type BindTexture = Gl3BindTexture;
    type CompileShader = Gl3CompileProgram;
}
