struct Real : Setting {
  DeclareClass(Real, "setting.real")

  Real(string name = {}, f64 value = {}, std::function<void (f64)> modify = {}) : Setting(name) {
    _currentValue = value;
    _latchedValue = value;
    _modify = modify;
  }

  auto modify(f64 value) const -> void { if(_modify) return _modify(value); }
  auto value() const -> f64 { return _currentValue; }
  auto latch() const -> f64 { return _latchedValue; }

  auto setModify(std::function<void (f64)> modify) { _modify = modify; }

  auto setValue(f64 value) -> void {
    if(!_allowedValues.empty() && !std::ranges::count(_allowedValues, value)) return;
    _currentValue = value;
    if(_dynamic) setLatch();
  }

  auto setLatch() -> void override {
    if(_latchedValue == _currentValue) return;
    _latchedValue = _currentValue;
    modify(_latchedValue);
  }

  auto setAllowedValues(std::vector<f64> allowedValues) -> void {
    _allowedValues = allowedValues;
    if(!_allowedValues.empty() && !std::ranges::count(_allowedValues, _currentValue)) setValue(_allowedValues.front());
  }

  auto readValue() const -> string override { return value(); }
  auto readLatch() const -> string override { return latch(); }
  auto readAllowedValues() const -> std::vector<string> override {
    std::vector<string> values;
    for(auto value : _allowedValues) values.push_back(value);
    return values;
  }
  auto writeValue(string value) -> void override { setValue(value.real()); }

protected:
  std::function<void (f64)> _modify;
  f64 _currentValue = {};
  f64 _latchedValue = {};
  std::vector<f64> _allowedValues;
};
