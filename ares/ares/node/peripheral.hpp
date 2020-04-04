struct Peripheral : Object {
  DeclareClass(Peripheral, "Peripheral")
  using Object::Object;

  auto manifest() -> string { if(_manifest) return _manifest(); return {}; }

  auto setManifest(function<string()> manifest) -> void { _manifest = manifest; }

protected:
  function<string ()> _manifest;
};
