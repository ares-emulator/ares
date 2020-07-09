auto Disc::command(u8 operation) -> void {
//print("* CDC ", hex(operation, 2L), "\n");

  switch(operation) {
  case 0x01: return commandGetStatus();
  case 0x13: return commandGetFirstAndLastTrackNumbers();
  case 0x14: return commandGetTrackStart();
  case 0x19: return commandTest();
  case 0x1a: return commandGetID();
  }
}

//0x01
auto Disc::commandGetStatus() -> void {
  uint8 data;
  data.bit(0) = ssr.error;
  data.bit(1) = ssr.motorOn;
  data.bit(2) = ssr.seekError;
  data.bit(3) = ssr.idError;
  data.bit(4) = ssr.shellOpen;
  data.bit(5) = ssr.reading;
  data.bit(6) = ssr.seeking;
  data.bit(7) = ssr.playingCDDA;
  fifo.response.write(data);
  ssr.shellOpen = 0;

  irq.acknowledge.delay = 3;
}

//0x13
auto Disc::commandGetFirstAndLastTrackNumbers() -> void {
  fifo.response.write(0x00);
  fifo.response.write(0x01);
  fifo.response.write(0x01);

  irq.acknowledge.delay = 3;
}

//0x14
auto Disc::commandGetTrackStart() -> void {
  fifo.response.write(0x00);
  fifo.response.write(0x00);
  fifo.response.write(0x00);

  irq.acknowledge.delay = 3;
}

//0x19
auto Disc::commandTest() -> void {
  u8 operation = fifo.parameter.read();
  switch(operation) {
  case 0x20: return commandTestControllerDate();
  }
}

//0x19 0x20
auto Disc::commandTestControllerDate() -> void {
  fifo.response.write(0x95);
  fifo.response.write(0x05);
  fifo.response.write(0x16);
  fifo.response.write(0xc1);

  irq.acknowledge.delay = 3;
}

//0x1a
auto Disc::commandGetID() -> void {
/*
  fifo.response.write(0x0a);
  fifo.response.write(0x90);
  fifo.response.write(0x00);
  fifo.response.write(0x00);
  fifo.response.write(0x00);
  fifo.response.write(0x00);
  fifo.response.write(0x00);
  fifo.response.write(0x00);
*/

  irq.acknowledge.delay = 3;
  irq.error.delay = 3;
}
