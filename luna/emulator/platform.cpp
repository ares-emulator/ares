auto Emulator::attach(ares::Node::Object node) -> void {
  if(interface && node->is<ares::Node::Screen>()) {
    screens = root->find<ares::Node::Screen>();
  }

  if(interface && node->is<ares::Node::Stream>()) {
    streams = root->find<ares::Node::Stream>();
  }
}

auto Emulator::detach(ares::Node::Object node) -> void {
  if(interface && node->is<ares::Node::Screen>()) {
    screens = root->find<ares::Node::Screen>();
  }

  if(interface && node->is<ares::Node::Stream>()) {
    streams = root->find<ares::Node::Stream>();
  }

  if(auto location = node->attribute("location")) {
    file::write({location, "settings.bml"}, node->save());
  }
}

auto Emulator::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  auto location = node->attribute("location");

  if(name == "manifest.bml") {
    if(!file::exists({location, name}) && directory::exists(location)) {
      if(auto manifest = execute("mia", "--system", node->name(), "--manifest", location).output) {
        return vfs::memory::open(manifest.data<uint8_t>(), manifest.size());
      }
    }
  }

  if(auto result = vfs::disk::open({location, name}, mode)) return result;

  if(required) {
    //attempt to pull required system firmware (boot ROMs, etc) from system template
    if(location == emulator.system.data) {
      if(file::exists({emulator.system.templates, name})) {
        file::copy({emulator.system.templates, name}, {emulator.system.data, name});
        if(auto result = vfs::disk::open({location, name}, mode)) return result;
      }
    }

    if(MessageDialog()
    .setTitle("Warning")
    .setText({"Missing required file:\n",
              location, name, "\n\n",
              "Would you like to browse for this file now?"})
    .setAlignment(Alignment::Center)
    .question() == "No") return {};

    if(auto source = BrowserDialog()
    .setTitle({"Load ", name})
    .setPath(location)
    .setAlignment(Alignment::Center)
    .openFile()
    ) {
      if(auto input = vfs::memory::open(source, true)) {
        if(auto output = file::open({location, name}, file::mode::write)) {
          output.write({input->data(), (uint)input->size()});
        }
      }
      if(auto result = vfs::disk::open({location, name}, mode)) return result;
    }
  }

  return {};
}

auto Emulator::event(ares::Event event) -> void {
  if(event == ares::Event::Power) {
    events.power = true;
  }
}

auto Emulator::log(string_view message) -> void {
  if(!system.log) {
    directory::create({"/tmpfs/Logs/", system.name, "/"});
    string datetime = chrono::local::datetime().transform("-: ", "  _").replace(" ", "");
    system.log.open({"/tmpfs/Logs/", system.name, "/event-", datetime, ".log"}, file::mode::write);
  }
  system.log.print(message);
}

auto Emulator::video(ares::Node::Screen node, const uint32_t* data, uint pitch, uint width, uint height) -> void {
  if(requests.captureScreenshot) {
    requests.captureScreenshot = false;
    captureScreenshot(data, pitch, width, height);
  }

  uint videoWidth = node->width() * node->scaleX();
  uint videoHeight = node->height() * node->scaleY();

  if(settings.video.aspectCorrection) {
    videoWidth = videoWidth * node->aspectX() / node->aspectY();
  }

  if(node->rotation() == 90 || node->rotation() == 270) {
    swap(videoWidth, videoHeight);
  }

  auto [viewportWidth, viewportHeight] = videoInstance.size();

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
  if(auto [output, length] = videoInstance.acquire(width, height); output) {
    length >>= 2;
    for(auto y : range(height)) {
      memory::copy<uint32>(output + y * length, data + y * pitch, width);
    }
    videoInstance.release();
    videoInstance.output(outputWidth, outputHeight);
  }

  static uint frameCounter = 0;
  static uint64_t previous, current;
  frameCounter++;

  current = chrono::timestamp();
  if(current != previous) {
    previous = current;
    setCaption({frameCounter, " fps"});
    frameCounter = 0;
  }
}

auto Emulator::audio(ares::Node::Stream) -> void {
  if(!streams) return;  //should never occur

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
        //stereo mixing
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
    audioInstance.output(samples);
  }
}

auto Emulator::input(ares::Node::Input input) -> void {
  inputManager.poll();

  bool allow = program.viewport.focused();
  if(settings.input.unfocused == "Allow") allow = true;
  if(videoInstance.exclusive()) allow = true;

  if(auto button = input->cast<ares::Node::Button>()) {
    button->setValue(0);
    if(auto instance = button->attribute<shared_pointer<InputButton>>("instance")) {
      if(allow) button->setValue(instance->value());
    }
  }

  if(auto axis = input->cast<ares::Node::Axis>()) {
    axis->setValue(0);
    if(auto instance = axis->attribute<shared_pointer<InputAxis>>("instance")) {
      if(allow) axis->setValue(instance->value());
    }
  }
}
