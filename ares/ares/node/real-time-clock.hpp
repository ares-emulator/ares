struct RealTimeClock : Object {
  DeclareClass(RealTimeClock, "Real Time Clock")
  using Object::Object;

  auto update() -> void { if(_update) return _update(); }
  auto timestamp() const -> uint64 { return _timestamp; }

  auto setUpdate(function<void ()> update) { _update = update; }
  auto setTimestamp(uint64 timestamp) -> void { _timestamp = timestamp; }

  auto synchronize(uint64 timestamp = 0) -> void {
    if(!timestamp) timestamp = chrono::timestamp();
    _timestamp = timestamp;
    update();
  }

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
    output.append(depth, "  timestamp: ", _timestamp, "\n");
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
    _timestamp = node["timestamp"].natural();
  }

protected:
  function<void ()> _update;
  uint64 _timestamp;
};
