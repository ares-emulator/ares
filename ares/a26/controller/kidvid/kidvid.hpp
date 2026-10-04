#include "audio.hpp"

struct KidVid : Controller, Thread {
  KidVid(Node::Port);
  ~KidVid();

  auto main() -> void;
  auto power(bool reset) -> void override;
  auto poll() -> void override;
  auto frame() -> void override;
  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto serialize(serializer&) -> void override;

private:
  enum class Profile : u32 { Unknown, Smurfs, Bears };

  auto rewind() -> void;
  auto select(u32 game) -> void;
  auto tapeIndex() const -> u32;
  auto tapeFile() const -> KidVidAudio::File;
  auto advanceCassette() -> void;
  auto startNextClip() -> void;
  auto updatePlayback() -> void;

  Node::Input::Button game1;
  Node::Input::Button game2;
  Node::Input::Button game3;
  Node::Input::Button skip;
  Node::Audio::Stream stream;
  VFS::Pak pak;
  KidVidAudio audio;

  Profile profile = Profile::Unknown;
  u32 tape = 0;
  u32 bit = 0;
  u32 bitsRemaining = 48;
  u32 block = 0;
  u32 song = 0;
  u32 fallbackFrames = 0;
  bool run = true;
  bool data = true;
  bool tapeBusy = false;
  bool songPlaying = false;
  bool beep = false;
  bool mediaAvailable = false;
};
