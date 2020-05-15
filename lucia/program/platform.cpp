auto Program::attach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Screen>()) {
    screens = emulator->root->find<ares::Node::Screen>();
  }

  if(auto stream = node->cast<ares::Node::Stream>()) {
    streams = emulator->root->find<ares::Node::Stream>();
    stream->setResamplerFrequency(ruby::audio.frequency());
  }
}

auto Program::detach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Screen>()) {
    screens = emulator->root->find<ares::Node::Screen>();
  }

  if(auto stream = node->cast<ares::Node::Stream>()) {
    streams = emulator->root->find<ares::Node::Stream>();
  }
}

auto Program::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  return emulator->open(node, name, mode, required);
}

auto Program::event(ares::Event event) -> void {
}

auto Program::log(string_view message) -> void {
  if(traceLogger.traceToTerminal.checked()) {
    return print(message);
  }
  if(!traceLogger.fp) {
    auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
    auto location = emulator->locate({Location::notsuffix(emulator->game.location), "-", datetime, ".log"}, ".log", settings.paths.debugging);
    traceLogger.fp.open(location, file::mode::write);
  }
  traceLogger.fp.print(message);
}

auto Program::video(ares::Node::Screen node, const uint32_t* data, uint pitch, uint width, uint height) -> void {
  if(!screens) return;

  if(requestScreenshot) {
    requestScreenshot = false;
    captureScreenshot(data, pitch, width, height);
  }

  if(node->width() != emulator->latch.width || node->height() != emulator->latch.height || node->rotation() != emulator->latch.rotation) {
    emulator->latch.width = node->width();
    emulator->latch.height = node->height();
    emulator->latch.rotation = node->rotation();
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
  }

  uint videoWidth = node->width() * node->scaleX();
  uint videoHeight = node->height() * node->scaleY();
  if(settings.video.aspectCorrection) videoWidth = videoWidth * node->aspectX() / node->aspectY();
  if(node->rotation() == 90 || node->rotation() == 270) swap(videoWidth, videoHeight);

  auto [viewportWidth, viewportHeight] = ruby::video.size();
  uint multiplierX = viewportWidth / videoWidth;
  uint multiplierY = viewportHeight / videoHeight;
  uint multiplier = min(multiplierX, multiplierY);

  uint outputWidth = videoWidth * multiplier;
  uint outputHeight = videoHeight * multiplier;

  if(multiplier == 0 || settings.video.output == "Scale") {
    float multiplierX = (float)viewportWidth / (float)videoWidth;
    float multiplierY = (float)viewportHeight / (float)videoHeight;
    float multiplier = min(multiplierX, multiplierY);

    outputWidth = videoWidth * multiplier;
    outputHeight = videoHeight * multiplier;
  }

  if(settings.video.output == "Stretch") {
    outputWidth = viewportWidth;
    outputHeight = viewportHeight;
  }

  pitch >>= 2;
  if(auto [output, length] = ruby::video.acquire(width, height); output) {
    length >>= 2;
    for(auto y : range(height)) {
      memory::copy<uint32_t>(output + y * length, data + y * pitch, width);
    }
    ruby::video.release();
    ruby::video.output(outputWidth, outputHeight);
  }

  static uint64_t frameCounter = 0, previous, current;
  frameCounter++;

  current = chrono::timestamp();
  if(current != previous) {
    previous = current;
    presentation.statusRight.setText({frameCounter, " FPS"});
    frameCounter = 0;
  }
}

auto Program::audio(ares::Node::Stream node) -> void {
  if(!streams) return;

  //process all pending frames (there may be more than one waiting)
  while(true) {
    //only process a frame if all streams have at least one pending frame
    for(auto& stream : streams) {
      if(!stream->pending()) return;
    }

    //mix all frames together
    double samples[2] = {0.0, 0.0};
    for(auto& stream : streams) {
      double buffer[2];
      uint channels = stream->read(buffer);
      if(channels == 1) {
        //monaural -> stereo mixing
        samples[0] += buffer[0];
        samples[1] += buffer[0];
      } else {
        samples[0] += buffer[0];
        samples[1] += buffer[1];
      }
    }

    //apply volume, balance, and clamping to the output frame
    double volume = !settings.audio.mute ? settings.audio.volume : 0.0;
    double balance = settings.audio.balance;
    for(uint c : range(2)) {
      samples[c] = max(-1.0, min(+1.0, samples[c] * volume));
      if(balance < 0.0) samples[1] *= 1.0 + balance;
      if(balance > 0.0) samples[0] *= 1.0 - balance;
    }

    //send frame to the audio output device
    ruby::audio.output(samples);
  }
}

auto Program::input(ares::Node::Input node) -> void {
  if(!driverSettings.inputDefocusAllow.checked()) {
    if(!ruby::video.fullScreen() && !presentation.focused()) {
      //todo: set node->value() to zero here
      return;
    }
  }

  emulator->input(node);
}
