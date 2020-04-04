struct Instruction : Tracer {
  DeclareClass(Instruction, "Instruction")

  Instruction(string name = {}, string component = {}) : Tracer(name, component) {
    setDepth(_depth);
  }

  auto depth() const -> uint { return _depth; }
  auto addressBits() const -> uint { return _addressBits; }

  auto setDepth(uint depth) -> void {
    _depth = depth;
    _history.reset();
    _history.resize(depth);
    for(auto& history : _history) history = ~0ull;
  }

  auto setAddressBits(uint addressBits) -> void {
    _addressBits = addressBits;
  }

  auto address(uint64 address) -> bool {
    _address = address;
    if(!_depth) return false;

    for(auto history : _history) {
      if(address == history) {
        _omitted++;
        return false;
      }
    }

    for(auto index : range(_depth - 1)) {
      _history[index] = _history[index + 1];
    }
    _history.last() = _address;

    return true;
  }

  auto notify(string_view instruction, string_view context, string_view extra = {}) -> void {
    if(!enabled()) return;

    if(_omitted) {
      PlatformLog({
      //_component, " ",
        "[Omitted: ", _omitted, "]\n"}
      );
      _omitted = 0;
    }

    string output{
    //_component, "  ",
      hex(_address, _addressBits + 3 >> 2), "  ",
      instruction, "  ",
      context, "  ",
      extra
    };
    PlatformLog({output.strip(), "\n"});
  }

  auto serialize(string& output, string depth) -> void override {
    Tracer::serialize(output, depth);
    output.append(depth, "  depth: ", _depth, "\n");
    output.append(depth, "  addressBits: ", _addressBits, "\n");
  }

  auto unserialize(Markup::Node node) -> void override {
    Tracer::unserialize(node);
    _depth = node["depth"].natural();
    _addressBits = node["addressBits"].natural();

    setDepth(_depth);
  }

protected:
  uint _depth = 4;
  uint _addressBits = 0;

  uint64 _address = 0;
  uint64 _omitted = 0;
  vector<uint64> _history;
};
