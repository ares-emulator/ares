struct ControllerMux {
  ControllerMux(void);
  auto select(n1 port) -> void;
  auto read() -> n6 {
    if(port_selected == 0) {
      return controllerPort1.read();
    } else {
      return controllerPort2.read();
    }
  };

  b1 port_selected;
};

extern ControllerMux controllerMux;
