struct Notification : Event {
  DeclareClass(Notification, "Notification")

  Notification(string name = {}, string component = {}) : Event(name, component) {
  }

  auto notify(string_view message = {}) -> void {
    if(!enabled()) return;

    if(message) {
      PlatformLog({_component, " [", _name, ": ", message, "]\n"});
    } else {
      PlatformLog({_component, " [", _name, "]\n"});
    }
  }

protected:
};
