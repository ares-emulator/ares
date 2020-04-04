struct Input : Object {
  DeclareClass(Input, "Input")
  using Object::Object;
};

struct Button : Input {
  DeclareClass(Button, "Button")
  using Input::Input;

  auto value() const -> boolean { return _value; }
  auto setValue(boolean value) -> void { _value = value; }

protected:
  boolean _value;
};

struct Axis : Input {
  DeclareClass(Axis, "Axis")
  using Input::Input;

  auto value() const -> integer { return _value; }
  auto setValue(integer value) -> void { _value = value; }

protected:
  integer _value;
  integer _minimum = -32768;
  integer _maximum = +32767;
};

struct Trigger : Input {
  DeclareClass(Trigger, "Trigger")
  using Input::Input;

  auto value() const -> integer { return _value; }
  auto setValue(integer value) -> void { _value = value; }

protected:
  integer _value;
  integer _minimum =      0;
  integer _maximum = +32767;
};

struct Rumble : Input {
  DeclareClass(Rumble, "Rumble")
  using Input::Input;

  auto enable() const -> boolean { return _enable; }
  auto setEnable(boolean enable) -> void { _enable = enable; }

protected:
  boolean _enable;
};
