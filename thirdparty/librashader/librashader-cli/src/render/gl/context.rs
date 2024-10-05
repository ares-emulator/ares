// Copyright (c) 2023 Christian Vallentin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

use anyhow::anyhow;
use glfw::{fail_on_errors, Context, Glfw, OpenGlProfileHint, PWindow, WindowHint, WindowMode};
use std::sync::Arc;

pub struct GlfwContext {
    _wnd: PWindow,
    _glfw: Glfw,
    pub gl: Arc<glow::Context>,
}

impl GlfwContext {
    pub fn new(version: GLVersion, width: u32, height: u32) -> Result<Self, anyhow::Error> {
        let mut glfw = glfw::init(fail_on_errors!())?;

        let GLVersion(major, minor) = version;
        glfw.window_hint(WindowHint::ContextVersion(major, minor));
        glfw.window_hint(WindowHint::OpenGlProfile(OpenGlProfileHint::Core));
        glfw.window_hint(WindowHint::OpenGlForwardCompat(true));
        glfw.window_hint(WindowHint::Visible(false));
        glfw.window_hint(WindowHint::OpenGlDebugContext(true));

        let (mut wnd, _events) = glfw
            .create_window(width, height, env!("CARGO_PKG_NAME"), WindowMode::Windowed)
            .ok_or_else(|| anyhow!("No window"))?;

        wnd.make_current();

        let gl =
            unsafe { glow::Context::from_loader_function(|proc| wnd.get_proc_address(proc) as _) };

        // unsafe {
        //     gl.enable(glow::DEBUG_OUTPUT);
        //     gl.enable(glow::DEBUG_OUTPUT_SYNCHRONOUS);
        //     gl.debug_message_callback(debug_callback);
        //     gl.debug_message_control(glow::DONT_CARE, glow::DONT_CARE, glow::DONT_CARE, &[], true);
        // }

        Ok(Self {
            _wnd: wnd,
            _glfw: glfw,
            gl: Arc::new(gl),
        })
    }
}

#[derive(Debug)]
pub struct GLVersion(pub u32, pub u32);
