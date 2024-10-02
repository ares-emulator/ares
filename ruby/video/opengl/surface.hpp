auto OpenGLSurface::size(u32 w, u32 h) -> void {
  if(width == w && height == h) return;
  width = w, height = h;
  w = glrSize(w), h = glrSize(h);

  if(texture) { glDeleteTextures(1, &texture); texture = 0; }
  if(buffer) { delete[] buffer; buffer = nullptr; }

  buffer = new u32[w * h]();
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, getFormat(), getType(), buffer);
}

auto OpenGLSurface::release() -> void {
  if(texture) { glDeleteTextures(1, &texture); texture = 0; }
  if(framebuffer) { glDeleteFramebuffers(1, &framebuffer); framebuffer = 0; }
  width = 0, height = 0;
}

auto OpenGLSurface::render(u32 sourceWidth, u32 sourceHeight, u32 targetX, u32 targetY, u32 targetWidth, u32 targetHeight) -> void {
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if(_chain != NULL) {
    libra_image_gl_t input = {texture, format, sourceWidth, sourceHeight};
    libra_image_gl_t output = {framebufferTexture, framebufferFormat, targetWidth, targetHeight};

    if (auto error = _libra.gl_filter_chain_frame(&_chain, frameCount++, input, output, NULL, NULL, NULL)) {
      _libra.error_print(error);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, framebufferHeight, framebufferWidth, 0, targetX, targetY, framebufferWidth + targetX, framebufferHeight + targetY, GL_COLOR_BUFFER_BIT, filter);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  } else {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, framebufferHeight, framebufferWidth, 0, targetX, targetY, targetWidth + targetX, targetHeight + targetY, GL_COLOR_BUFFER_BIT, filter);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  }

}
