ControllerMux controllerMux;

ControllerMux::ControllerMux(void) {
  port_selected = 0;
}

auto ControllerMux::select(n1 port) -> void {
  port_selected = port;
}
