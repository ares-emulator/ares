struct GraphicsFrame : Debugger {
  DeclareClass(GraphicsFrame, "debugger.graphics.frame")

  struct Image {
    u32 width = 0;
    u32 height = 0;
    std::vector<u32> pixels;
  };

  GraphicsFrame(string name = {}) : Debugger(name) {}

  auto requestCapture() const -> void { if(_requestCapture) _requestCapture(); }
  auto captureValid() const -> bool { return _captureValid ? _captureValid() : false; }
  auto commandCount() const -> u32 { return _commandCount ? _commandCount() : 0; }
  auto commandText(u32 index) const -> string { return _commandText ? _commandText(index) : string{}; }
  //Decoded operands for the command, summarized on one line.
  auto commandArguments(u32 index) const -> string { return _commandArguments ? _commandArguments(index) : string{}; }
  auto commandDetail(u32 index) const -> string { return _commandDetail ? _commandDetail(index) : string{}; }
  auto commandOpcode(u32 index) const -> u32 { return _commandOpcode ? _commandOpcode(index) : 0; }
  auto commandType(u32 index) const -> u32 { return _commandType ? _commandType(index) : 0; }
  auto views() const -> std::vector<string> { return _views ? _views() : std::vector<string>{}; }
  //Command indices worth a scrollback entry: sync commands whose rendered
  //buffers differ from the previous entry's.
  auto ticks() const -> std::vector<u32> { return _ticks ? _ticks() : std::vector<u32>{}; }
  auto render(u32 command, u32 view) const -> Image { return _render ? _render(command, view) : Image{}; }
  auto state(u32 command) const -> string { return _state ? _state(command) : string{}; }
  auto summary() const -> string { return _summary ? _summary() : string{}; }

  auto setRequestCapture(std::function<void ()> callback) -> void { _requestCapture = callback; }
  auto setCaptureValid(std::function<bool ()> callback) -> void { _captureValid = callback; }
  auto setCommandCount(std::function<u32 ()> callback) -> void { _commandCount = callback; }
  auto setCommandText(std::function<string (u32)> callback) -> void { _commandText = callback; }
  auto setCommandArguments(std::function<string (u32)> callback) -> void { _commandArguments = callback; }
  auto setCommandDetail(std::function<string (u32)> callback) -> void { _commandDetail = callback; }
  auto setCommandOpcode(std::function<u32 (u32)> callback) -> void { _commandOpcode = callback; }
  auto setCommandType(std::function<u32 (u32)> callback) -> void { _commandType = callback; }
  auto setViews(std::function<std::vector<string> ()> callback) -> void { _views = callback; }
  auto setTicks(std::function<std::vector<u32> ()> callback) -> void { _ticks = callback; }
  auto setRender(std::function<Image (u32, u32)> callback) -> void { _render = callback; }
  auto setState(std::function<string (u32)> callback) -> void { _state = callback; }
  auto setSummary(std::function<string ()> callback) -> void { _summary = callback; }

  auto serialize(string& output, string depth) -> void override {
    Debugger::serialize(output, depth);
  }

  auto unserialize(Markup::Node node) -> void override {
    Debugger::unserialize(node);
  }

private:
  std::function<void ()> _requestCapture;
  std::function<bool ()> _captureValid;
  std::function<u32 ()> _commandCount;
  std::function<string (u32)> _commandText;
  std::function<string (u32)> _commandArguments;
  std::function<string (u32)> _commandDetail;
  std::function<u32 (u32)> _commandOpcode;
  std::function<u32 (u32)> _commandType;
  std::function<std::vector<string> ()> _views;
  std::function<std::vector<u32> ()> _ticks;
  std::function<Image (u32, u32)> _render;
  std::function<string (u32)> _state;
  std::function<string ()> _summary;
};
