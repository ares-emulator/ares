struct System : Object {
  DeclareClass(System, "system")
  using Object::Object;

  auto game() -> string { if(_game) return _game(); return {}; }
  auto run() -> void { if(_run) return _run(); }
  auto power(bool reset = false) -> void { if(_power) return _power(reset); }
  auto save() -> void { if(_save) return _save(); }
  auto unload() -> void { if(_unload) return _unload(); }
  auto serialize(bool synchronize = true) -> serializer { if(_serialize) return _serialize(synchronize); return {}; }
  auto unserialize(serializer& s) -> bool { if(_unserialize) return _unserialize(s); return false; }

  auto setGame(std::function<string ()> game) -> void { _game = game; }
  auto setRun(std::function<void ()> run) -> void { _run = run; }
  auto setPower(std::function<void (bool)> power) -> void { _power = power; }
  auto setSave(std::function<void ()> save) -> void { _save = save; }
  auto setUnload(std::function<void ()> unload) -> void { _unload = unload; }
  auto setSerialize(std::function<serializer (bool)> serialize) -> void { _serialize = serialize; }
  auto setUnserialize(std::function<bool (serializer&)> unserialize) -> void { _unserialize = unserialize; }

protected:
  std::function<string ()> _game;
  std::function<void ()> _run;
  std::function<void (bool)> _power;
  std::function<void ()> _save;
  std::function<void ()> _unload;
  std::function<serializer (bool)> _serialize;
  std::function<bool (serializer&)> _unserialize;
};
