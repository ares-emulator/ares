struct Port : Object {
  DeclareClass(Port, "Port")

  Port(string name = {}) : Object(name) {
    setAllocate([&] {
      return Node::Peripheral::create();
    });
  }

  inline auto allocate() -> Node::Peripheral { if(_allocate) return _allocate(); return {}; }
  inline auto attach(Node::Peripheral node) -> void { if(_attach) return _attach(node); }
  inline auto detach(Node::Peripheral node) -> void { if(_detach) return _detach(node); }
  inline auto type() const -> string { return _type; }
  inline auto family() const -> string { return _family; }
  inline auto hotSwappable() const -> bool { return _hotSwappable; }

  auto setAllocate(function<Node::Peripheral ()> allocate) -> void { _allocate = allocate; }
  auto setAttach(function<void (Node::Peripheral)> attach) -> void { _attach = attach; }
  auto setDetach(function<void (Node::Peripheral)> detach) -> void { _detach = detach; }
  auto setType(string_view type) -> void { _type = type; }
  auto setFamily(string_view family) -> void { _family = family; }
  auto setHotSwappable(bool hotSwappable) -> void { _hotSwappable = hotSwappable; }

  auto connected() -> Node::Peripheral {
    return find<Node::Peripheral>(0);
  }

  auto connect(Node::Peripheral peripheral) -> void {
    disconnect();
    attach(peripheral);
  }

  //searches a source port tree for a peripheral that matches this peripheral.
  //if found, it will connect the peripheral to its parent port.
  auto scan(Node::Object port) -> void {
    disconnect();
    if(!port) return;
    if(auto from = port->find(shared())) {
      if(auto node = from->find<Node::Peripheral>(0)) {
        connect(node);
      }
    }
  }

  auto disconnect() -> void {
    if(auto peripheral = connected()) {
      detach(peripheral);
      remove(peripheral);
    }
  }

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
    output.append(depth, "  type: ", _type, "\n");
    output.append(depth, "  family: ", _family, "\n");
    output.append(depth, "  hotSwappable: ", _hotSwappable, "\n");
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
    _type = node["type"].string();
    _family = node["family"].string();
    _hotSwappable = node["hotSwappable"].boolean();
  }

protected:
  function<Node::Peripheral ()> _allocate;
  function<void (Node::Peripheral)> _attach;
  function<void (Node::Peripheral)> _detach;
  string _type;
  string _family;
  bool _hotSwappable = false;
};
