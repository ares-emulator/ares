struct Properties : Object {
  DeclareClass(Properties, "Properties")

  Properties(string name = {}) : Object(name) {
  }

  auto query() const -> string { if(_query) return _query(); return {}; }

  auto setQuery(function<string ()> query) -> void { _query = query; }

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
  }

protected:
  function<string ()> _query;
};
