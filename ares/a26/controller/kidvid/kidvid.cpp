#include "tables.hpp"

static constexpr const char* KidVidSmurfsSha256 =
  "bbe0f1d952aab334de82db18cd84feb49a7fd9beb60a39c52658483564af78be";
static constexpr const char* KidVidBearsSha256 =
  "6161ac39cddfe7773c553f1b7a5925b703025ebe6ac18e934b8eb94465ed4548";

KidVid::KidVid(Node::Port parent) {
  node = parent->append<Node::Peripheral>("KidVid Voice Module");
  game1 = node->append<Node::Input::Button>("Game 1");
  game2 = node->append<Node::Input::Button>("Game 2");
  game3 = node->append<Node::Input::Button>("Game 3");
  skip = node->append<Node::Input::Button>("Skip");

  stream = node->append<Node::Audio::Stream>("KidVid");
  stream->setChannels(1);
  stream->setFrequency(16000);

  node->setPak(pak = platform->pak(node));
  audio.load(pak);
  if(cartridge.sha256() == KidVidSmurfsSha256) profile = Profile::Smurfs;
  if(cartridge.sha256() == KidVidBearsSha256) profile = Profile::Bears;
  rewind();
  Thread::create(16000, std::bind_front(&KidVid::main, this));
}

KidVid::~KidVid() {
  Thread::destroy();
  stream.reset();
  pak.reset();
}

auto KidVid::main() -> void {
  stream->frame(audio.clock());
  Thread::step(1);
  Thread::synchronize(cpu);
}

auto KidVid::power(bool reset) -> void {
  if(!reset || profile == Profile::Smurfs) rewind();
  Thread::create(16000, std::bind_front(&KidVid::main, this));
}

auto KidVid::poll() -> void {
  platform->input(game1);
  platform->input(game2);
  platform->input(game3);
  platform->input(skip);
}

auto KidVid::frame() -> void {
  if(profile == Profile::Unknown) return;
  if(profile == Profile::Smurfs && system.controls.reset->value()) rewind();

  if(skip->value() && song && KidVidData::ClipOrder[song - 1] != 0
    && KidVidData::ClipOrder[song - 1] != 11) {
    audio.stop();
  }

  if(!tape) {
    if(game1->value()) select(1);
    else if(game2->value()) select(2);
    else if(game3->value()) select(3);
  }

  if(tape && run && !tapeBusy) advanceCassette();
  updatePlayback();
}

auto KidVid::read() -> n8 {
  n8 lines = 0xff;
  lines.bit(3) = data;
  return lines;
}

auto KidVid::write(n8 lines) -> void {
  run = lines.bit(0);
}

auto KidVid::serialize(serializer& s) -> void {
  Thread::serialize(s);
  auto audioStateCompatible = audio.serialize(s);
  s(tape);
  s(bit);
  s(bitsRemaining);
  s(block);
  s(song);
  s(fallbackFrames);
  s(run);
  s(data);
  s(tapeBusy);
  s(songPlaying);
  s(beep);

  if(s.reading()) {
    auto cassetteBits = KidVidData::CassetteData.size() * 8;
    if(tape > 4 || bit >= cassetteBits || !bitsRemaining || bitsRemaining > 48
      || bitsRemaining > cassetteBits - bit || song > KidVidData::ClipOrder.size() || fallbackFrames > 92) {
      rewind();
    }
    mediaAvailable = tape && audio.available(KidVidAudio::File::Shared) && audio.available(tapeFile());
    if(!audioStateCompatible) {
      mediaAvailable = false;
      songPlaying = false;
      beep = true;
      tapeBusy = tape != 0;
      fallbackFrames = tape ? 92 : 0;
    }
  }
}

auto KidVid::rewind() -> void {
  audio.reset();
  tape = 0;
  bit = profile == Profile::Bears ? 48 : 0;
  bitsRemaining = 48;
  block = 0;
  song = 0;
  fallbackFrames = 0;
  data = true;
  tapeBusy = false;
  songPlaying = false;
  beep = false;
  mediaAvailable = false;
}

auto KidVid::select(u32 game) -> void {
  if(game < 1 || game > 3) return;
  tape = game == 1 ? 2 : game == 2 ? 3 : profile == Profile::Bears ? 4 : 1;
  bit = profile == Profile::Bears ? 48 : 0;
  bitsRemaining = 48;
  block = 0;
  song = KidVidData::FirstClip[tapeIndex()];
  fallbackFrames = 0;
  tapeBusy = false;
  songPlaying = false;
  beep = false;
  mediaAvailable = audio.available(KidVidAudio::File::Shared) && audio.available(tapeFile());
}

auto KidVid::tapeIndex() const -> u32 {
  if(tape == 4) return 3;
  return profile == Profile::Smurfs ? tape - 1 : tape + 2;
}

auto KidVid::tapeFile() const -> KidVidAudio::File {
  return (KidVidAudio::File)tapeIndex();
}

auto KidVid::advanceCassette() -> void {
  auto byte = KidVidData::CassetteData[bit >> 3];
  data = byte >> (7 - (bit & 7)) & 1;
  bit++;
  if(--bitsRemaining) return;

  if(block == 0) {
    bit = 96 + (tape - 1) * 48;
  } else if(block >= KidVidData::BlockCounts[tapeIndex()]) {
    bit = 336;
  } else {
    bit = 288;
    startNextClip();
  }
  block++;
  bitsRemaining = 48;
}

auto KidVid::startNextClip() -> void {
  if(!mediaAvailable || song >= KidVidData::ClipOrder.size()) {
    beep = true;
    tapeBusy = true;
    songPlaying = false;
    fallbackFrames = 92;
    return;
  }

  auto entry = KidVidData::ClipOrder[song++];
  beep = !(entry & 0x80);
  auto clip = entry & 0x7f;
  if(clip + 1 >= KidVidData::ClipBoundaries.size()) {
    mediaAvailable = false;
    return startNextClip();
  }

  auto begin = KidVidData::ClipBoundaries[clip];
  auto end = KidVidData::ClipBoundaries[clip + 1];
  if(end < begin + 786) {
    mediaAvailable = false;
    return startNextClip();
  }
  auto file = clip < 10 ? KidVidAudio::File::Shared : tapeFile();
  if(!audio.play(file, begin, end - 786)) {
    mediaAvailable = false;
    return startNextClip();
  }
  songPlaying = true;
  tapeBusy = true;
}

auto KidVid::updatePlayback() -> void {
  if(mediaAvailable) {
    if(!songPlaying) return;
    auto remaining = audio.remaining();
    tapeBusy = remaining > 262 * 60 || !beep;
    if(remaining) return;
    songPlaying = false;
    tapeBusy = !beep;
    if(!beep) startNextClip();
    return;
  }

  if(fallbackFrames) {
    fallbackFrames--;
    tapeBusy = fallbackFrames > 60;
  }
}
