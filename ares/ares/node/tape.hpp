struct Tape : Peripheral {
  DeclareClass(Tape, "tape") using Peripheral::Peripheral;

  ~Tape() {
    unload();
  }

  auto play() -> void { _playing = true; }
  auto record() -> void { _recording = true; }
  auto stop() -> void {
    _playing = false;
    _recording = false;
  }

  auto playing() const -> bool { return _playing; }
  auto recording() const -> bool { return _recording; }

  auto loaded() const -> bool { return _loaded; }
  auto load() -> bool {
    if (!_loaded && _load && _load()) {
      _loaded = true;
      return true;
    }

    return _loaded;
  }
  auto unload() -> void {
    if (_loaded) {
      if (_unload) _unload();
      _loaded = false;
    }
  }

  auto setLoad(std::function<bool()> load) -> void { _load = load; }
  auto setUnload(std::function<void()> unload) -> void { _unload = unload; }

  auto setSupportPlay(bool support) -> void { _supportPlay = support; }
  auto setSupportRecord(bool support) -> void { _supportRecord = support; }

  auto supportPlay() const -> bool { return _supportPlay; }
  auto supportRecord() const -> bool { return _supportRecord; }

  auto length() const -> u64 { return _length; }
  auto setLength(u64 length) -> void { _length = length; }

  auto position() const -> u64 { return _position; }
  auto setPosition(u64 position) -> void { _position = position; }

  auto frequency() const -> u64 { return _frequency; }
  auto setFrequency(u64 frequency) -> void { _frequency = frequency; }

  auto serialize(string &output, string depth) -> void override {
    Peripheral::serialize(output, depth);
    output.append(depth, "  supportPlay: ", _supportPlay, "\n");
    output.append(depth, "  supportRecord: ", _supportRecord, "\n");
    output.append(depth, "  playing: ", _playing, "\n");
    output.append(depth, "  recording: ", _recording, "\n");
  }
  auto unserialize(Markup::Node node) -> void override {
    Peripheral::unserialize(node);
    _supportPlay = node["supportPlay"].boolean();
    _supportRecord = node["supportRecord"].boolean();
    _playing = node["playing"].boolean();
    _recording = node["recording"].boolean();
  }

private:
  bool _supportPlay = false;
  bool _supportRecord = false;
  bool _playing = false;
  bool _recording = false;
  bool _loaded = false;
  u64 _length = 0;
  u64 _position = 0;
  u64 _frequency = 1;
  std::function<bool()> _load;
  std::function<void()> _unload;
};
