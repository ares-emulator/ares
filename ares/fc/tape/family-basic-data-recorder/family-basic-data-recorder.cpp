FamilyBasicDataRecorder::FamilyBasicDataRecorder(Node::Port parent) {
  node = parent->append<Node::Tape>("Family BASIC Data Recorder");
  node->setSupportPlay(true);
  node->setSupportRecord(true);
  node->setLoad([&] { return load(); });
  node->setUnload([&] { unload(); });

  data.reset();
  output = 0;
  input = 0;
  speed = 1;
  node->setPosition(0);
  node->setLength(0);
  node->setFrequency(1);
  enable = false;
  Thread::create(system.frequency(), std::bind_front(&FamilyBasicDataRecorder::main, this));
}

FamilyBasicDataRecorder::~FamilyBasicDataRecorder() {
  Thread::destroy();
  node.reset();
}

auto FamilyBasicDataRecorder::read() -> n1 {
  return output;
}

auto FamilyBasicDataRecorder::write(n3 data) -> void {
  enable = data.bit(2);
  input  = data.bit(0);
}

auto FamilyBasicDataRecorder::load() -> bool {
  if (!node->setPak(pak = platform->pak(node))) return false;
  auto fd = pak->read("program.tape");
  if(!fd) return false;
  node->setPosition(0);
  node->setLength(pak->attribute("length").natural());
  node->setFrequency(pak->attribute("frequency").natural());
  data.allocate(node->length());
  data.load(fd);
  fd.reset();
  output = 0;
  range = pak->attribute("range").natural();
  speed = system.frequency() / node->frequency();
  return true;
}

auto FamilyBasicDataRecorder::unload() -> void {
  if (!pak) return;

  auto fd = pak->write("program.tape");
  fd->resize(data.size() * sizeof(u64));
  data.save(fd);
  fd.reset();

  pak.reset();
  data.reset();
  output = 0;
  input = 0;
  range = 0;
  speed = 1;
  node->setFrequency(1);
  node->setPosition(0);
  node->setLength(0);
}

auto FamilyBasicDataRecorder::main() -> void {
  if (!node->playing() && !node->recording())
    return step(speed);

  u64 position = node->position();
  u64 length = node->length();

  if (node->playing()) {
    if (position > length) {
      node->stop();
      return step(speed);
    }

    if (!enable) {
      output = 0;
      return step(speed);
    }

    output = data.read(position++) > range / 2 ? 1 : 0;
    node->setPosition(position);
    return step(speed);
  }

  if (node->recording()) {
    u64 frequency = node->frequency();
    if (length <= position) {
      length = (position / frequency + 1) * frequency;
      auto fd = pak->write("program.tape");
      fd->resize(length);
      data.save(fd);

      data.allocate(length, 0);
      data.load(fd);
      fd.reset();
    }

    data[position++] = (range >> 1) + (input ? 1 : 0);
    node->setPosition(position);
    node->setLength(length);
    return step(speed);
  }
}

auto FamilyBasicDataRecorder::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}