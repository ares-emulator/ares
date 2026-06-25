#include <spec/spec.hpp>

namespace ares::ZXSpectrum {

#include "deck.cpp"
#include "tray.cpp"

auto Tape::allocate(Node::Port parent) -> Node::Peripheral {
  node = parent->append<Node::Tape>("ZX Spectrum Tape");
  node->setSupportPlay(true);
  node->setSupportRecord(false);
  node->setLoad([&] { return load(); });
  node->setUnload([&] { unload(); });
  node->setPosition(0);
  node->setLength(0);
  node->setFrequency(44100);

  range = 0;
  output = 0;
  input = 0;

  stream = node->append<Node::Audio::Stream>("Audio");
  stream->setChannels(1);
  stream->setFrequency(node->frequency());
  Thread::create(node->frequency(), [&] { Tape::main(); });

  return node;
}

auto Tape::connect() -> bool {
  return node && node->load();
}

auto Tape::load() -> bool {
  if(!node->setPak(pak = platform->pak(node))) return false;

  information = {};
  information.title = pak->attribute("title");

  range = pak->attribute("range").natural();
  pak->setAttribute("modified", false);
  node->setFrequency(pak->attribute("frequency").natural());
  node->setLength(pak->attribute("length").natural());
  node->setPosition(0);
  node->setSupportRecord(pak->attribute("writable").boolean());
  stream->setFrequency(node->frequency());
  Thread::setFrequency(node->frequency());

  if(node->length() != 0) {
    auto fd = pak->read("program.tape");
    if(!fd) return false;
    data.allocate(node->length());
    data.load(fd);
    fd.reset();
  }

  return true;
}

auto Tape::main() -> void {
  u64 position = node->position();
  u64 length = node->length();

  if(node->playing()) {
    if(position >= length) {
      output = 0;
      node->stop();
      step(1);
      return;
    }

    u64 sample = data.read(position++);
    node->setPosition(position);

    stream->frame((float)sample / (float)range);
    output = sample > (range / 2);
    step(1);
    return;
  }

  if(node->recording()) {
    if(length <= position) {
      auto fd = pak->write("program.tape");
      fd->resize(data.size() * sizeof(u64));
      data.save(fd);
      fd.reset();

      length = (position / node->frequency() + 1) * node->frequency();
      fd = pak->read("program.tape");
      data.allocate(length, 0);
      data.load(fd);
      fd.reset();
      node->setLength(length);
    }

    data[position++] = (range >> 1) + (input ? 1 : 0);
    pak->setAttribute("modified", true);
    node->setPosition(position);
    stream->frame(input ? 1.0f : 0.0f);
    step(1);
    return;
  }

  output = 0;
  stream->frame(0.0f);
  step(1);
}

auto Tape::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto Tape::disconnect() -> void {
  if(!node) return;
  node->unload();
  Thread::destroy();
  stream = {};
  node = {};
}

auto Tape::unload() -> void {
  if(!pak) return;

  if(pak->attribute("modified").boolean() && data.size() != 0) {
    auto fd = pak->write("program.tape");
    fd->resize(data.size() * sizeof(u64));
    data.save(fd);
    fd.reset();
  }

  data.reset();
  information = {};
  pak.reset();
  node->setSupportRecord(false);
  node->setFrequency(44100);
  stream->setFrequency(node->frequency());
  Thread::setFrequency(node->frequency());
  node->setPosition(0);
  node->setLength(0);
  range = 0;
  output = 0;
  input = 0;
}

auto Tape::read() -> n1 {
  if(!node || !node->playing()) return 0;
  return output;
}

auto Tape::write(n1 data) -> void {
  if(!node || !node->recording()) return;
  input = data;
}

auto Tape::serialize(serializer& s) -> void {
  u64 position = node ? node->position() : 0;
  u64 length = node ? node->length() : 0;
  u64 frequency = node ? node->frequency() : 44100;
  bool playing = node ? node->playing() : false;
  bool recording = node ? node->recording() : false;

  Thread::serialize(s);
  s(position);
  s(length);
  s(frequency);
  s(playing);
  s(recording);
  s(range);
  s(output);
  s(input);
  s(data);

  if(s.reading() && node) {
    node->setPosition(position);
    node->setLength(length);
    node->setFrequency(frequency);
    stream->setFrequency(frequency);
    Thread::setFrequency(frequency);
    node->stop();
    if(playing) node->play();
    if(recording) node->record();
  }
}

}
