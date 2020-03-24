struct Emulator : ares::Platform {
  ares::Node::Object root;

  //emulator.cpp
  auto create(shared_pointer<ares::Interface>, string location) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto quit() -> void;
  auto power(bool on) -> void;
  auto validateConfiguration(Markup::Node, Markup::Node) -> void;

  auto connected(string location) -> ares::Node::Port;

  //platform.cpp
  auto attach(ares::Node::Object) -> void override;
  auto detach(ares::Node::Object) -> void override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto event(ares::Event) -> void override;
  auto log(string_view message) -> void override;
  auto video(ares::Node::Screen, const uint32_t* data, uint pitch, uint width, uint height) -> void override;
  auto audio(ares::Node::Stream) -> void override;
  auto input(ares::Node::Input) -> void override;

  //video.cpp
  auto videoUpdate() -> void;
  auto videoUpdateColors() -> void;
  auto videoUpdateShader() -> void;
  auto videoToggleFullscreen() -> void;

  //audio.cpp
  auto audioUpdate() -> void;
  auto audioUpdateEffects() -> void;

  //input.cpp
  auto inputUpdate() -> void;

  //states.cpp
  auto saveState(uint slot) -> bool;
  auto loadState(uint slot) -> bool;

  //status.cpp
  auto updateMessage() -> void;
  auto showMessage(const string& message = {}) -> void;
  auto setCaption(const string& caption = {}) -> void;

  //utility.cpp
  auto captureScreenshot(const uint32_t* data, uint pitch, uint width, uint height) -> void;

  struct System {
    string name;
    string data;
    string templates;
    bool power = false;
    file_buffer log;
  } system;

  struct Events {
    bool power = false;
  } events;

  struct Requests {
    bool captureScreenshot = false;
  } requests;

  struct State {
    struct Message {
      uint64_t timestamp = 0;
      string text;
    } message;
  } state;

  vector<ares::Node::Screen> screens;
  vector<ares::Node::Stream> streams;
};

extern Emulator emulator;
