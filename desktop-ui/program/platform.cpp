auto Program::attach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Video::Screen>()) {
    screens = emulator->root->find<ares::Node::Video::Screen>();
  }

  if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
    streams = emulator->root->find<ares::Node::Audio::Stream>();
    stream->setResamplerFrequency(ruby::audio.frequency());
  }
}

auto Program::detach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Video::Screen>()) {
    screens = emulator->root->find<ares::Node::Video::Screen>();
    screens.removeByValue(screen);
  }

  if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
    streams = emulator->root->find<ares::Node::Audio::Stream>();
    streams.removeByValue(stream);
    stream->setResamplerFrequency(ruby::audio.frequency());
  }
}

auto Program::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  return emulator->pak(node);
}

auto Program::event(ares::Event event) -> void {
}

auto Program::log(string_view message) -> void {
  if(traceLogger.traceToTerminal.checked()) {
    print(message);
  }
  if(traceLogger.traceToFile.checked()) {
    if(!traceLogger.fp) {
      auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
      auto location = emulator->locate({Location::notsuffix(emulator->game->location), "-", datetime, ".log"}, ".log", settings.paths.debugging);
      traceLogger.fp.open(location, file::mode::write);
    }
    traceLogger.fp.print(message);
    //if the trace log file size grows beyond 1GiB, close the file handle so a new log file will be started.
    //this is done because few text editors can open logs beyond a certain file size.
    if(traceLogger.fp.size() >= 1_GiB) {
      traceLogger.fp.close();
    }
  }
}

auto Program::video(ares::Node::Video::Screen node, const u32* data, u32 pitch, u32 width, u32 height) -> void {
  if(!screens) return;

  if(requestScreenshot) {
    requestScreenshot = false;
    captureScreenshot(data, pitch, width, height);
  }

  if(node->width() != emulator->latch.width || node->height() != emulator->latch.height || node->rotation() != emulator->latch.rotation) {
    emulator->latch.width = node->width();
    emulator->latch.height = node->height();
    emulator->latch.rotation = node->rotation();
    emulator->latch.changed = true;  //signal Program::main() to potentially resize the presentation window
  }

  u32 videoWidth = node->width() * node->scaleX();
  u32 videoHeight = node->height() * node->scaleY();
  if(settings.video.aspectCorrection) videoWidth = videoWidth * node->aspectX() / node->aspectY();
  if(node->rotation() == 90 || node->rotation() == 270) swap(videoWidth, videoHeight);

  ruby::video.lock();
  auto [viewportWidth, viewportHeight] = ruby::video.size();
  u32 multiplierX = viewportWidth / videoWidth;
  u32 multiplierY = viewportHeight / videoHeight;
  u32 multiplier = min(multiplierX, multiplierY);

  u32 outputWidth = videoWidth * multiplier;
  u32 outputHeight = videoHeight * multiplier;

  if(multiplier == 0 || settings.video.output == "Scale") {
    f32 multiplierX = (f32)viewportWidth / (f32)videoWidth;
    f32 multiplierY = (f32)viewportHeight / (f32)videoHeight;
    f32 multiplier = min(multiplierX, multiplierY);

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
      memory::copy<u32>(output + y * length, data + y * pitch, width);
    }
    ruby::video.release();
    ruby::video.output(outputWidth, outputHeight);
  }
  ruby::video.unlock();

  static u64 frameCounter = 0, previous, current;
  frameCounter++;

  current = chrono::timestamp();
  if(current != previous) {
    previous = current;
    message.framesPerSecond = frameCounter;
    frameCounter = 0;
  }
}

auto Program::audio(ares::Node::Audio::Stream node) -> void {
  if(!streams) return;

  //process all pending frames (there may be more than one waiting)
  while(true) {
    //only process a frame if all streams have at least one pending frame
    for(auto& stream : streams) {
      if(!stream->pending()) return;
    }

    //mix all frames together
    f64 samples[2] = {0.0, 0.0};
    for(auto& stream : streams) {
      f64 buffer[2];
      u32 channels = stream->read(buffer);
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
    f64 volume = !settings.audio.mute ? settings.audio.volume : 0.0;
    f64 balance = settings.audio.balance;
    for(u32 c : range(2)) {
      samples[c] = max(-1.0, min(+1.0, samples[c] * volume));
      if(balance < 0.0) samples[1] *= 1.0 + balance;
      if(balance > 0.0) samples[0] *= 1.0 - balance;
    }

    //send frame to the audio output device
    ruby::audio.output(samples);
  }
}

auto Program::input(ares::Node::Input::Input node) -> void {
  if(!driverSettings.inputDefocusAllow.checked()) {
    if(!ruby::video.fullScreen() && !presentation.focused()) {
      //treat the input as not being active
      if(auto button = node->cast<ares::Node::Input::Button>()) button->setValue(0);
      if(auto axis = node->cast<ares::Node::Input::Axis>()) axis->setValue(0);
      if(auto trigger = node->cast<ares::Node::Input::Trigger>()) trigger->setValue(0);
      return;
    }
  }

  emulator->input(node);
}
