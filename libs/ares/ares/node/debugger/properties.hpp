struct Properties : Debugger {
  DeclareClass(Properties, "debugger.properties")

  Properties(string name = {}) : Debugger(name) {
  }

  auto query() const -> string { if(_query) return _query(); return {}; }

  auto setQuery(std::function<string ()> query) -> void { _query = query; }

  auto serialize(string& output, string depth) -> void override {
    Debugger::serialize(output, depth);
  }

  auto unserialize(Markup::Node node) -> void override {
    Debugger::unserialize(node);
  }

protected:
  std::function<string ()> _query;
};
