FamicomDataRecorder::FamicomDataRecorder(Node::Port port) {
  node = port->append<Node::Tape>("Famicom Data Recorder");

  node->setSupportPlay(true);
  node->setSupportRecord(true);
  node->setLoad([&] { return load(); });
  node->setUnload([&] { unload(); });

  node->setPosition(0);
  node->setLength(0);
  node->setFrequency(44100);

  data.reset();
  output = 0;
  input = 0;
  speed = system.frequency() / node->frequency();

  Thread::create(system.frequency(), std::bind_front(&FamicomDataRecorder::main, this));
}

FamicomDataRecorder::~FamicomDataRecorder() {
  Thread::destroy();
  node.reset();
}

auto FamicomDataRecorder::read() -> n1 {
  if (!node->playing())
    return 0;
  return output;
}

auto FamicomDataRecorder::write(n1 data) -> void {
  if (!node->recording())
    return;
  input = data;
}

auto FamicomDataRecorder::load() -> bool {
  if (!node->setPak(pak = platform->pak(node)))
    return false;

  node->setPosition(0);
  node->setLength(pak->attribute("length").natural());
  node->setFrequency(pak->attribute("frequency").natural());

  if (node->length() != 0) {
    auto fd = pak->read("program.tape");
    if (!fd)
      return false;
    data.allocate(node->length());
    data.load(fd);
    fd.reset();
  }

  range = pak->attribute("range").natural();
  speed = system.frequency() / node->frequency();

  return true;
}

auto FamicomDataRecorder::unload() -> void {
  if (!pak)
    return;

  if (data.size() != 0) {
    auto fd = pak->write("program.tape");
    fd->resize(data.size() * sizeof(u64));
    data.save(fd);
    fd.reset();
  }

  node->setFrequency(44100);
  node->setPosition(0);
  node->setLength(0);

  pak.reset();
  output = 0;
  input = 0;
  range = 0;
  speed = system.frequency() / 44100;
  data.reset();
}

auto FamicomDataRecorder::main() -> void {
  u64 position = node->position();
  u64 length = node->length();

  if (node->playing()) {
    if (position >= length) {
      node->stop();
      return step();
    }

    output = data.read(position++) > range / 2 ? 1 : 0;
    node->setPosition(position);
    return step();
  }

  if (node->recording()) {
    u64 frequency = node->frequency();
    if (length <= position) {
      auto fd = pak->write("program.tape");
      fd->resize(data.size() * sizeof(u64));
      data.save(fd);
      fd.reset();

      length = (position / frequency + 1) * frequency;
      fd = pak->read("program.tape");
      data.allocate(length, 0);
      data.load(fd);
      fd.reset();
      node->setLength(length);
    }

    data[position++] = (range >> 1) + (input ? 1 : 0);
    node->setPosition(position);
    return step();
  }

  return step();
}

auto FamicomDataRecorder::step() -> void {
  Thread::step(speed);
  Thread::synchronize();
}
