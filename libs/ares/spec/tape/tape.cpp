#include <spec/spec.hpp>

namespace ares::ZXSpectrum {

#include "deck.cpp"
#include "tray.cpp"

auto Tape::allocate(Node::Port parent) -> Node::Peripheral {
  node = parent->append<Node::Peripheral>("ZX Spectrum Tape");
  position = 0;
  length = 0;
  range = 0;

  return node;
}

auto Tape::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  auto fd = pak->read("program.tape");
  if(!fd) return disconnect();

  data.allocate(fd->size());
  data.load(fd);
  fd.reset();

  range = pak->attribute("range").natural();
  frequency = pak->attribute("frequency").natural();
  length = pak->attribute("length").natural();

  stream = node->append<Node::Audio::Stream>("Audio");
  stream->setChannels(1);
  stream->setFrequency(frequency);
  Thread::create(frequency, [&] { Tape::main(); });
}

auto Tape::main() -> void {
  if (!tapeDeck.state.playing) {
    stream->frame(0.0f);
    step(1);
    return;
  }

  if (position > length) {
    tapeDeck.state.playing = 0;
    tapeDeck.play->setValue(false);
    return;
  }

  u64 sample = data.read(position);
  position++;

  stream->frame((float)sample / (float)range);
  tapeDeck.state.output = sample > (range / 2);

  step(1);
}

auto Tape::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto Tape::disconnect() -> void {
  if(!node) return;
  Thread::destroy();
  data.reset();
  node = {};
  position = 0;
  length = 0;
}

}
