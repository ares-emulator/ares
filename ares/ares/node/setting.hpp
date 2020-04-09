struct Settings : Object {
  DeclareClass(Settings, "Settings")
  using Object::Object;
};

struct Setting : Object {
  DeclareClass(Setting, "Setting")
  using Object::Object;

  auto dynamic() const -> bool { return _dynamic; }

  auto setDynamic(bool dynamic) -> void {
    _dynamic = dynamic;
  }

  virtual auto setLatch() -> void {}

  virtual auto readValue() const -> string { return {}; }
  virtual auto readLatch() const -> string { return {}; }
  virtual auto readAllowedValues() const -> vector<string> { return {}; }
  virtual auto writeValue(string value) -> void {}

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
    output.append(depth, "  dynamic: ", _dynamic, "\n");
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
    _dynamic = node["dynamic"].boolean();
  }

protected:
  bool _dynamic = false;
};

template<typename Cast, typename Type> struct Abstract : Setting {
  using Setting::Setting;

  Abstract(string name = {}, Type value = {}, function<void (Type)> modify = {}) : Setting(name), _defaultValue(value) {
    _currentValue = value;
    _latchedValue = value;
    if constexpr(is_same_v<Type, boolean>) _allowedValues = {false, true};
    _modify = modify;
  }

  auto modify(Type value) const -> void { if(_modify) return _modify(value); }
  auto value() const -> Type { return _currentValue; }
  auto latch() const -> Type { return _latchedValue; }

  auto setModify(function<void (Type)> modify) {
    _modify = modify;
  }

  auto setValue(Type value) -> void {
    if(_allowedValues && !_allowedValues.find(value)) return;
    _currentValue = value;
    if(_dynamic) _latchedValue = _currentValue;
    modify(value);
  }

  auto setLatch() -> void override {
    _latchedValue = _currentValue;
  }

  auto setAllowedValues(vector<Type> allowedValues) -> void {
    _allowedValues = allowedValues;
    //if the current setting isn't an allowed value, change it to the first allowed value
    if(_allowedValues && !_allowedValues.find(_currentValue)) {
      _currentValue = _allowedValues.first();
      _latchedValue = _currentValue;
    }
  }

  auto readValue() const -> string override { return value(); }
  auto readLatch() const -> string override { return latch(); }
  auto readAllowedValues() const -> vector<string> override {
    vector<string> values;
    for(auto& value : _allowedValues) values.append(value);
    return values;
  }

  auto writeValue(string value) -> void override {
    if(_allowedValues && !readAllowedValues().find(value)) return;
    if constexpr(is_same_v<Type, boolean>) _currentValue = value.boolean();
    if constexpr(is_same_v<Type, natural>) _currentValue = value.natural();
    if constexpr(is_same_v<Type, integer>) _currentValue = value.integer();
    if constexpr(is_same_v<Type, real   >) _currentValue = value.real();
    if constexpr(is_same_v<Type, string >) _currentValue = value;
    if(_dynamic) _latchedValue = _currentValue;
    modify(_currentValue);
  }

  auto load(Node::Object source) -> bool override {
    if(!Setting::load(source)) return false;
    if(auto setting = source->cast<shared_pointer<Cast>>()) {
      if(!_allowedValues || _allowedValues.find(setting->_currentValue)) {
        _currentValue = setting->_currentValue;
        _latchedValue = _currentValue;
        modify(_currentValue);
      }
    }
    return true;
  }

  auto serialize(string& output, string depth) -> void override {
    Setting::serialize(output, depth);
    output.append(depth, "  value: ", _currentValue, "\n");
    if(_latchedValue) output.append(depth, "  latch: ", _latchedValue, "\n");
  }

  auto unserialize(Markup::Node node) -> void override {
    Setting::unserialize(node);
    if constexpr(is_same_v<Type, boolean>) _currentValue = node["value"].boolean(), _latchedValue = node["latch"].boolean();
    if constexpr(is_same_v<Type, natural>) _currentValue = node["value"].natural(), _latchedValue = node["latch"].natural();
    if constexpr(is_same_v<Type, integer>) _currentValue = node["value"].integer(), _latchedValue = node["latch"].integer();
    if constexpr(is_same_v<Type, real>)    _currentValue = node["value"].real(),    _latchedValue = node["latch"].real();
    if constexpr(is_same_v<Type, string>)  _currentValue = node["value"].text(),    _latchedValue = node["latch"].text();
  }

protected:
  function<void (Type)> _modify;
  const Type _defaultValue;
  Type _currentValue;
  Type _latchedValue;
  vector<Type> _allowedValues;
};

struct Boolean : Abstract<Boolean, boolean> {
  DeclareClass(Boolean, "Boolean")
  using Abstract::Abstract;

  auto copy(Node::Object object) -> void override {
    if(auto source = object->cast<Node::Boolean>()) {
      Object::copy(source);
      setValue(source->value());
      setLatch();
    }
  }
};

struct Natural : Abstract<Natural, natural> {
  DeclareClass(Natural, "Natural")
  using Abstract::Abstract;

  auto copy(Node::Object object) -> void override {
    if(auto source = object->cast<Node::Natural>()) {
      Object::copy(source);
      setValue(source->value());
      setLatch();
    }
  }
};

struct Integer : Abstract<Integer, integer> {
  DeclareClass(Integer, "Integer")
  using Abstract::Abstract;

  auto copy(Node::Object object) -> void override {
    if(auto source = object->cast<Node::Integer>()) {
      Object::copy(source);
      setValue(source->value());
      setLatch();
    }
  }
};

struct Real : Abstract<Real, real> {
  DeclareClass(Real, "Real")
  using Abstract::Abstract;

  auto copy(Node::Object object) -> void override {
    if(auto source = object->cast<Node::Real>()) {
      Object::copy(source);
      setValue(source->value());
      setLatch();
    }
  }
};

struct String : Abstract<String, string> {
  DeclareClass(String, "String")
  using Abstract::Abstract;

  auto copy(Node::Object object) -> void override {
    if(auto source = object->cast<Node::String>()) {
      Object::copy(source);
      setValue(source->value());
      setLatch();
    }
  }
};
