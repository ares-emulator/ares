struct ControllerPort {
  Node::Port port;
  std::unique_ptr<Controller> device;

  //port.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  ControllerPort(string name);
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void { device.reset(); }

  auto update() -> void;

  auto readControl() -> n8 { return control; }
  auto writeControl(n8 data) -> void { control = data; update(); }

  auto readData() -> n8 {
    if(device) device->poll();
    update();
    return dataLines;
  }
  auto writeData(n8 data) -> void { dataLatch = data; update(); }

  // TODO: Implement working serial transfers. This code is mostly placeholder.

  // S-CTRL write bits
  //   b7,6: Serial baud rate: 0==4800bps, 1==2400bps, 2==1200bps, 3==300bps
  //   b5: Enable serial IN
  //   b4: Enable serial OUT
  //   b3: Enable interrupt on RxdReady
  auto writeSerialControl(n8 data) -> void { serialControl.bit(7,3) = data.bit(7,3); }
  // S-CTRL read bits
  //   b2: RxdError
  //   b1: RxdReady (data available)
  //   b0: TxdFull
  auto readSerialControl() -> n8 { return serialControl; }

  auto readSerialTxData() -> n8 { return serialTxBuffer; }
  auto writeSerialTxData(n8 data) -> void {
  // TxdFull is not set here to allow dummy serial writes (needed to boot The Animals!)
    serialTxBuffer = data;
    // serialControl.bit(0) = 1; // TxdFull
  }

  auto readSerialRxData() -> n8 { return serialRxBuffer; }
  auto writeSerialRxData(n8 data) -> void {
  // Writes to Rx buffer are ignored (unconfirmed)
    // serialRxBuffer = data;
  }

  auto power(bool reset) -> void;
  auto serialize(serializer&) -> void;

protected:
  const string name;
  n8 control;  //d0-d6 = PC0-PC6 (0 = input; 1 = output); d7 = TH-INT enable
  n8 dataLatch;
  n8 dataLines;
  n8 serialControl;
  n8 serialTxBuffer;
  n8 serialRxBuffer;
  friend struct Controller;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;
extern ControllerPort extensionPort;
