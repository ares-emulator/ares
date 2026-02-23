//horrible implementation of run-time introspection:
//allow converting a unique class string to a derived Node type.

struct Class {
  struct Instance {
    const string identifier;
    const std::function<Node::Object ()> create;
  };

  static auto classes() -> std::vector<Instance>& {
    static std::vector<Instance> classes;
    return classes;
  }

  template<typename T> static auto register() -> void {
    auto& cls = classes();
    auto it = std::ranges::find_if(cls, [&](const Instance& instance) { return instance.identifier == T::identifier(); });
    if(it == cls.end()) {
      cls.push_back({T::identifier(), &T::create});
    } else {
      throw;
    }
  }

  static auto create(string identifier) -> Node::Object {
    auto& cls = classes();
    auto it = std::ranges::find_if(cls, [&](const Instance& instance) { return instance.identifier == identifier; });
    if(it != cls.end()) return it->create();
    if(identifier == "Object") throw;  //should never occur: detects unregistered classes
    return create("Object");
  }

  template<typename T> struct Register {
    Register() { Class::register<T>(); }
  };
};
