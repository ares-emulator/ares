struct Event : Object {
  DeclareClass(Event, "Event")

  Event(string name = {}, string component = {}) : Object(name) {
    _component = component;
  }

  inline auto component() const -> string { return _component; }
  inline auto enabled() const -> bool { return _enabled; }

  auto setComponent(string component) -> void { _component = component; }
  auto setEnabled(bool enabled) -> void { _enabled = enabled; }

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
    output.append(depth, "  component: ", _component, "\n");
    output.append(depth, "  enabled: ", _enabled, "\n");
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
    _component = node["component"].string();
    _enabled = node["enabled"].boolean();
  }

protected:
  string _component;
  bool _enabled = false;
};
