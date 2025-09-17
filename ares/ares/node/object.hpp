//identifier() is static, allowing template<typename T> to access via T::identifier()
//identity() is virtual, allowing T* to access via T->identity()

#define DeclareClass(Type, Name) \
  static auto identifier() -> string { return Name; } \
  static auto create() -> Node::Object { return std::make_shared<Type>(); } \
  auto identity() const -> string override { return Name; } \
  private: static inline Class::Register<Type> register; public: \

struct Object : std::enable_shared_from_this<Object> {
  static auto identifier() -> string { return "Object"; }
  static auto create() -> Node::Object { return std::make_shared<Object>(); }
  virtual auto identity() const -> string { return "Object"; }
  private: static inline Class::Register<Object> register; public:
//DeclareClass(Object, "object")

  Object(string name = {}) : _name(name) {}
  virtual ~Object() = default;

  auto name() const -> string { return _name; }
  auto parent() const -> std::weak_ptr<Object> { return _parent; }

  auto setName(string_view name) -> void { _name = name; }

  auto prepend(Node::Object node) -> Node::Object {
    if(auto found = find(node)) return found;
    _nodes.insert(_nodes.begin(), node);
    node->_parent = shared_from_this();
    PlatformAttach(node);
    return node;
  }

  template<typename T, typename... P>
  auto prepend(P&&... p) -> std::shared_ptr<typename T::element_type> {
    using Type = typename T::element_type;
    auto node = std::make_shared<Type>(std::forward<P>(p)...);
    prepend(node);
    return node;
  }

  auto append(Node::Object node) -> Node::Object {
    if(auto found = find(node)) return found;
    _nodes.push_back(node);
    node->_parent = shared_from_this();
    PlatformAttach(node);
    return node;
  }

  template<typename T, typename... P>
  auto append(P&&... p) -> std::shared_ptr<typename T::element_type> {
    using Type = typename T::element_type;
    auto node = std::make_shared<Type>(std::forward<P>(p)...);
    append(node);
    return node;
  }

  auto remove(Node::Object node) -> void {
    if(std::erase(_nodes, node)) {
      PlatformDetach(node);
      node->reset();
      node->_parent.reset();
    }
  }

  auto reset() -> void {
    for(auto& node : _nodes) {
      PlatformDetach(node);
      node->reset();
      node->_parent.reset();
    }
    _nodes.clear();
  }

  template<typename T>
  auto cast() -> std::shared_ptr<typename T::element_type> {
    return std::dynamic_pointer_cast<typename T::element_type>(shared_from_this());
  }

  template<typename T>
  auto is() -> bool {
    return (bool)cast<T>();
  }

  template<typename T>
  auto find() -> std::vector<std::shared_ptr<typename T::element_type>> {
    std::vector<std::shared_ptr<typename T::element_type>> result;
    if(dynamic_cast<typename T::element_type*>(this)) {
      if(auto instance = cast<T>()) result.push_back(instance);
    }
    for(auto& node : _nodes) {
      auto sub = node->find<T>();
      std::ranges::copy(sub, std::back_inserter(result));
    }
    return result;
  }

  template<typename T>
  auto find(u32 index) -> std::shared_ptr<typename T::element_type> {
    auto result = find<T>();
    if(index < result.size()) return result[index];
    return {};
  }

  auto find(Node::Object source) -> Node::Object {
    if(!source) return {};
    for(auto& node : _nodes) {
      if(node->identity() == source->identity() && node->_name == source->_name) return node;
    }
    return {};
  }

  template<typename T = Node::Object>
  auto find(string name) -> T {
    using Type = typename T::element_type;
    auto path = nall::split(name, "/");
    if(path.empty()) return {};
    name = path.front();
    path.erase(path.begin());
    for(auto& node : _nodes) {
      if(node->_name != name) continue;
      if(!path.empty()) return node->find<T>(nall::merge(path, "/"));
      if(node->identity() == Type::identifier()) return node->template cast<T>();
    }
    return {};
  }

  template<typename T = Node::Object>
  auto scan(string name) -> T {
    using Type = typename T::element_type;
    for(auto& node : _nodes) {
      if(node->identity() == Type::identifier() && node->_name == name) return node->template cast<T>();
      if(auto result = node->scan<T>(name)) return result;
    }
    return {};
  }

  template<typename T>
  auto enumerate(std::vector<T>& objects) -> void {
    using Type = typename T::element_type;
    if(auto instance = cast<T>()) objects.push_back(instance);
    for(auto& node : _nodes) node->enumerate<T>(objects);
  }

  auto pak() -> VFS::Pak {
    return _pak;
  }

  auto setPak(VFS::Pak pak) -> bool {
    _pak = pak;
    return (bool)_pak;
  }

  template<typename T = string>
  auto attribute(const string& name) const -> T {
    if(auto attribute = _attributes.find(name)) {
      if(attribute->value.is<T>()) return attribute->value.get<T>();
    }
    return {};
  }

  template<typename T = string>
  auto hasAttribute(const string& name) const -> bool {
    if(auto attribute = _attributes.find(name)) {
      if(attribute->value.is<T>()) return true;
    }
    return false;
  }

  template<typename T = string, typename U = string>
  auto setAttribute(const string& name, const U& value = {}) -> void {
    if constexpr(is_same_v<T, string> && !is_same_v<U, string>) return setAttribute(name, string{value});
    if(auto attribute = _attributes.find(name)) {
      if((const T&)value) attribute->value = (const T&)value;
      else _attributes.remove(*attribute);
    } else {
      if((const T&)value) _attributes.insert({name, (const T&)value});
    }
  }

  virtual auto load(Node::Object source) -> bool {
    if(!source || identity() != source->identity() || _name != source->_name) return false;
    _attributes = source->_attributes;
    return true;
  }

  auto save() -> string {
    string markup;
    serialize(markup, {});
    return markup;
  }

  virtual auto serialize(string& output, string depth) -> void {
    output.append(depth, "node: ", identity(), "\n");
    output.append(depth, "  name: ", _name, "\n");
    for(auto& attribute : _attributes) {
      if(!attribute.value.is<string>()) continue;
      output.append(depth, "  attribute\n");
      output.append(depth, "    name: ", attribute.name, "\n");
      output.append(depth, "    value: ", attribute.value.get<string>(), "\n");
    }
    depth.append("  ");
    for(auto& node : _nodes) {
      node->serialize(output, depth);
    }
  }

  virtual auto unserialize(Markup::Node markup) -> void {
    if(!markup) return;
    _name = markup["name"].string();
    _attributes.reset();
    for(auto& attribute : markup.find("attribute")) {
      _attributes.insert({attribute["name"].string(), attribute["value"].string()});
    }
    for(auto& leaf : markup.find("node")) {
      auto node = Class::create(leaf.string());
      append(node);
      node->unserialize(leaf);
    }
  }

  virtual auto copy(Node::Object source) -> void {
    _attributes = source->_attributes;
    for(auto& from : source->_nodes) {
      for(auto& to : _nodes) {
        if(from->identity() != to->identity()) continue;
        if(from->name() != to->name()) continue;
        to->copy(from);
        break;
      }
    }
  }

  auto begin() { return _nodes.begin(); }
  auto end() { return _nodes.end(); }

protected:
  string _name;
  VFS::Pak _pak;
  set<Attribute> _attributes;
  std::weak_ptr<Object> _parent;
  std::vector<Node::Object> _nodes;
};
