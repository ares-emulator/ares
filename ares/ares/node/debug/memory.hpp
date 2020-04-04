struct Memory : Object {
  DeclareClass(Memory, "Memory")

  Memory(string name = {}) : Object(name) {
  }

  auto size() const -> uint { return _size; }
  auto read(uint32 address) const -> uint8 { if(_read) return _read(address); return 0; }
  auto write(uint32 address, uint8 data) const -> void { if(_write) return _write(address, data); }

  auto setSize(uint size) -> void { _size = size; }
  auto setRead(function<uint8 (uint32)> read) -> void { _read = read; }
  auto setWrite(function<void (uint32, uint8)> write) -> void { _write = write; }

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
  }

protected:
  uint _size = 0;
  function<uint8 (uint32)> _read;
  function<void (uint32, uint8)> _write;
};
