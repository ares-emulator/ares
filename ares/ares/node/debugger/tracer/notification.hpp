struct Notification : Tracer {
  DeclareClass(Notification, "debugger.tracer.notification")

  Notification(string name = {}, string component = {}) : Tracer(name, component) {
  }

  auto notify(const string& message = {}) -> void {
    if(!enabled()) return;
    PlatformLog(std::dynamic_pointer_cast<Tracer>(shared_from_this()), message);
  }

protected:
};
