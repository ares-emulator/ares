#include "transfer-pak.cpp"
#include "bio-sensor.cpp"

Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  port = node->append<Node::Port>("Pak");
  port->setFamily("Nintendo 64");
  port->setType("Pak");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setConnect([&] { return connect(); });
  port->setDisconnect([&] { return disconnect(); });
  port->setSupported({"Controller Pak", "Rumble Pak", "Transfer Pak", "Bio Sensor"});

  bank = 0;

  axis = node->append<Node::Input::Axis>("Axis");

  x           = node->append<Node::Input::Axis>  ("X-Axis");
  y           = node->append<Node::Input::Axis>  ("Y-Axis");
  up          = node->append<Node::Input::Button>("Up");
  down        = node->append<Node::Input::Button>("Down");
  left        = node->append<Node::Input::Button>("Left");
  right       = node->append<Node::Input::Button>("Right");
  b           = node->append<Node::Input::Button>("B");
  a           = node->append<Node::Input::Button>("A");
  cameraUp    = node->append<Node::Input::Button>("C-Up");
  cameraDown  = node->append<Node::Input::Button>("C-Down");
  cameraLeft  = node->append<Node::Input::Button>("C-Left");
  cameraRight = node->append<Node::Input::Button>("C-Right");
  l           = node->append<Node::Input::Button>("L");
  r           = node->append<Node::Input::Button>("R");
  z           = node->append<Node::Input::Button>("Z");
  start       = node->append<Node::Input::Button>("Start");
}

Gamepad::~Gamepad() {
  disconnect();
}

auto Gamepad::save() -> void {
  if(!slot) return;
  if(slot->name() == "Controller Pak") {
    ram.save(pak->write("save.pak"));
  }
  if(slot->name() == "Transfer Pak") {
    transferPak.save();
  }
}

auto Gamepad::allocate(string name) -> Node::Peripheral {
  if(name == "Controller Pak") return slot = port->append<Node::Peripheral>("Controller Pak");
  if(name == "Rumble Pak"    ) return slot = port->append<Node::Peripheral>("Rumble Pak");
  if(name == "Transfer Pak"  ) return slot = port->append<Node::Peripheral>("Transfer Pak");
  if(name == "Bio Sensor"    ) return slot = port->append<Node::Peripheral>("Bio Sensor");
  return {};
}

auto Gamepad::connect() -> void {
  if(!slot) return;
  pakDetectLatched = true;
  if(slot->name() == "Controller Pak") {
    bool create = true;

    node->setPak(pak = platform->pak(node));
    system.controllerPakBankCount = system.configuredControllerPakBankCount; //reset controller bank count
    ram.allocate(system.controllerPakBankCount * 32_KiB); //allocate N banks * 32KiB, max # of banks allowed is 62
    bank = 0;
    formatControllerPak();
    if(auto fp = pak->read("save.pak")) {
      if(fp->attribute("loaded").boolean()) {
        //read the bank count
        u8 banks;
        u32 bank_size;

        fp->seek(0x20 + 0x1A);
        fp->read(&banks, sizeof(banks));
        fp->seek(0);

        if (banks < 1) {
          banks = 1;
        } else if (banks > 62) {
          banks = 62;
        }

        bank_size = 32_KiB * banks;

        if (bank_size != ram.size) {
          ram.allocate(bank_size);

          //update the system controller bank count
          system.controllerPakBankCount = banks;
        }
        ram.load(pak->read("save.pak"));

        if (fp->size() != bank_size) {
          //reallocate vfs node
          pak->remove(fp);
          pak->append("save.pak", bank_size);
          ram.save(pak->write("save.pak")); //write data back to filesystem
        }

        create = false;
      }
    }

    if (create) {
      //we need to create a controller pak file, so reallocate the vfs file to configured size
      if (auto fp = pak->read("save.pak")) {
        pak->remove(fp);
        pak->append("save.pak", system.controllerPakBankCount * 32_KiB);
        ram.save(pak->write("save.pak"));
      }
    }
  }
  if(slot->name() == "Rumble Pak") {
    motor = node->append<Node::Input::Rumble>("Rumble");
  }
  if(slot->name() == "Transfer Pak") {
    transferPak.load(slot);
  }
  if(slot->name() == "Bio Sensor") {
    bioSensor.load();
    // Bio Sensor BPM setting node
    bioSensorBpm = slot->append<Node::Setting::Integer>(
      "Bio Sensor BPM",
      bioSensor.beatsPerMinute,
      [&](s64 value) { bioSensor.beatsPerMinute = value; }
    );
    bioSensorBpm->setDynamic(true);
    bioSensorBpm->setAllowedValues({30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180});
  }
}

auto Gamepad::disconnect() -> void {
  if(!slot) return;
  pakDetectLatched = true;
  if(slot->name() == "Controller Pak") {
    save();
    ram.reset();
  }
  if(slot->name() == "Rumble Pak") {
    rumble(false);
    node->remove(motor);
    motor.reset();
  }
  if(slot->name() == "Transfer Pak") {
    transferPak.unload();
  }
  if(slot->name() == "Bio Sensor") {
    bioSensor.unload();
    if(bioSensorBpm) {
      slot->remove(bioSensorBpm);
      bioSensorBpm.reset();
    }
  }
  port->remove(slot);
  slot.reset();
}

auto Gamepad::rumble(bool enable) -> void {
  if(!motor) return;
  motor->setEnable(enable);
  platform->input(motor);
}

auto Gamepad::comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 {
  b1 valid = 0;
  b1 over = 0;

  //status
  if(input[0] == 0x00 || input[0] == 0xff) {
    output[0] = 0x05;  //0x05 = gamepad; 0x02 = mouse
    output[1] = 0x00;
    if(slot) {
      if(pakDetectLatched) {
        output[2] = 0x03;  //0x03 = pak inserted
      } else {
        output[2] = 0x01;  //0x01 = pak present
      }
    } else {
      output[2] = 0x02;  //0x02 = pak absent
    }
    pakDetectLatched = false;  //reset flag after reporting pak status
    valid = 1;
  }

  //read controller state
  if(input[0] == 0x01) {
    u32 data = read();
    output[0] = data >> 24;
    output[1] = data >> 16;
    output[2] = data >>  8;
    output[3] = data >>  0;
    if(recv <= 4) {
      over = 0;
    } else {
      over = 1;
    }
    valid = 1;
  }

  //read pak
  if(input[0] == 0x02 && send >= 3 && recv >= 1) {
    u16 address = (input[1] << 8 | input[2] << 0) & ~31;

    //NUS-CNT only touches the first 33 bytes of output (32 data + 1 data CRC);
    //reads of less than 32 bytes are technically possible, but with no data CRC
    n8 recv_data_len = min(recv, 32);
    for(u32 index : range(min(recv, 33))) output[index] = 0x00; //zero out up to 33 bytes

    //check if no pak is connected, DETECT is latched, or address CRC is invalid
    b1 data_crc_no_pak = 0;
    if(!slot || pakDetectLatched || pif.addressCRC(address) != (n5)input[2]) {
      data_crc_no_pak = 1;
      valid = 1;
      goto read_pak_data_crc;
    }

    //controller pak
    if(ram) {
      for(u32 index : range(recv_data_len)) {
        // read into current bank
        if(address <= 0x7FFF) output[index] = ram.read<Byte>(bank * 32_KiB + address);
        else output[index] = 0;
        address++;
      }
      valid = 1;
    }

    //rumble pak
    if(motor) {
      for(u32 index : range(recv_data_len)) {
        if(address <= 0x7FFF) output[index] = 0;
        else if(address <= 0x8FFF) output[index] = 0x80;
        else output[index] = motor->enable() ? 0xFF : 0x00;
        address++;
      }
      valid = 1;
    }

    //transfer pak
    if(slot && slot->name() == "Transfer Pak") {
      for(u32 index : range(recv_data_len)) output[index] = transferPak.read(address++);
      valid = 1;
    }

    //bio sensor
    if(slot && slot->name() == "Bio Sensor") {
      bioSensor.update();
      for(u32 index : range(recv_data_len)) output[index] = bioSensor.read(address++);
      valid = 1;
    }

read_pak_data_crc:
    //calculate the data CRC if we have enough recv bytes
    if(valid && recv >= 33) {
      output[32] = pif.dataCRC({&output[0], 32});
      if (data_crc_no_pak) output[32] ^= 0xFF;
    }
  }

  //write pak
  if(input[0] == 0x03 && send >= 4 && recv >= 1) {
    u16 address = (input[1] << 8 | input[2] << 0) & ~31;
    n8 *send_data = &input[3];
    n8 send_data_len = min(send - 3, (n8)32); //max of 32 bytes can be written at once

    //check if no pak is connected, DETECT is latched, or address CRC is invalid
    b1 data_crc_no_pak = 0;
    if(!slot || pakDetectLatched || pif.addressCRC(address) != (n5)input[2]) {
      data_crc_no_pak = 1;
      valid = 1;
      goto write_pak_data_crc;
    }

    //controller pak
    if(ram) {
      //check if address is bank switch command
      if (address == 0x8000) {
        if (send >= 4) {
          u8 reqBank = input[3];
          if (reqBank < system.controllerPakBankCount) {
            bank = reqBank;
          }
        } else {
          if (system.homebrewMode) {
            debug(unusual, "Controller Pak bank switch command with no bank specified");
          }
          bank = 0;
        }

        if (system.homebrewMode) {
          //Verify we have 32 bytes (1 block) input and each value is the same bank
          if (send == 35) {
            u8 bank = input[3];
            for (u32 i = 4; i < 35; i++) {
              if (input[i] != bank) {
                debug(unusual, "Controller Pak bank switch command with mismatched data");
                break;
              }
            }
          } else {
            debug(unusual, "Controller Pak bank switch command with unusual data length");
          }
        }

        valid = 1;
      } else {
        for(u32 index : range(send_data_len)) {
          if(address <= 0x7FFF) ram.write<Byte>(bank * 32_KiB + address, send_data[index]);
          address++;
        }
        valid = 1;
      }
    }

    //rumble pak
    if(motor) {
      if(address >= 0xC000) rumble((*send_data) & 1);
      valid = 1;
    }

    //transfer pak
    if(slot && slot->name() == "Transfer Pak") {
      for(u32 index : range(send_data_len)) {
        transferPak.write(address++, send_data[index]);
      }
      valid = 1;
    }

    //bio sensor
    if(slot && slot->name() == "Bio Sensor") {
      //Bio Sensor is read-only; writes are ignored
      valid = 1;
    }

write_pak_data_crc:
    if(valid) {
      output[0] = 0x00; //zero out the data CRC
      //calculate the data CRC if we have enough send bytes
      if (send_data_len == 32) output[0] = pif.dataCRC({send_data, send_data_len});
      if (data_crc_no_pak) output[0] ^= 0xFF;
    }
  }

  n2 status = 0;
  status.bit(0) = valid;
  status.bit(1) = over;
  return status;
}

auto Gamepad::read() -> n32 {
  platform->input(x);
  platform->input(y);
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(b);
  platform->input(a);
  platform->input(cameraUp);
  platform->input(cameraDown);
  platform->input(cameraLeft);
  platform->input(cameraRight);
  platform->input(l);
  platform->input(r);
  platform->input(z);
  platform->input(start);

  auto cardinalMax   = 85.0;
  auto diagonalMax   = 69.0;
  auto innerDeadzone =  7.0; // default should remain 7 (~8.2% of 85) as the deadzone is axial in nature and fights cardinalMax
  auto saturationRadius = (innerDeadzone + diagonalMax + sqrt(pow(innerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * innerDeadzone)) / sqrt(2.0); //from linear response curve function within axis->processDeadzoneAndResponseCurve, substitute saturationRadius * sqrt(2) / 2 for right-hand lengthAbsolute and set diagonalMax as the result then solve for saturationRadius
  auto offset = 0.0;

  //scale {-32767 ... +32767} to {-saturationRadius + offset ... +saturationRadius + offset}
  auto ax = axis->setOperatingRange(x->value(), saturationRadius, offset);
  auto ay = axis->setOperatingRange(y->value(), saturationRadius, offset);

  //create inner axial dead-zone in range {-innerDeadzone ... +innerDeadzone} and scale from it up to saturationRadius
  ax = axis->processDeadzoneAndResponseCurve(ax, innerDeadzone, saturationRadius, offset);
  ay = axis->processDeadzoneAndResponseCurve(ay, innerDeadzone, saturationRadius, offset);

  auto scaledLength = hypot(ax - offset, ay - offset);
  if(scaledLength > saturationRadius) {
    ax = axis->revisePosition(ax, scaledLength, saturationRadius, offset);
    ay = axis->revisePosition(ay, scaledLength, saturationRadius, offset);
  }

  //let cardinalMax and diagonalMax define boundaries and restrict to an octagonal gate
  double axBounded = 0.0;
  double ayBounded = 0.0;
  axis->applyGateBoundaries(innerDeadzone, cardinalMax, diagonalMax, ax, ay, offset, axBounded, ayBounded);
  ax = axBounded;
  ay = ayBounded;

  //keep cardinal input within positive and negative bounds of cardinalMax
  ax = axis->clampAxisToNearestBoundary(ax, offset, cardinalMax);
  ay = axis->clampAxisToNearestBoundary(ay, offset, cardinalMax);

  //add epsilon to counteract floating point precision error
  ax = axis->counteractPrecisionError(ax);
  ay = axis->counteractPrecisionError(ay);
  
  n32 data;
  data.byte(0) = s8(-ay);
  data.byte(1) = s8(+ax);
  data.bit(16) = cameraRight->value();
  data.bit(17) = cameraLeft->value();
  data.bit(18) = cameraDown->value();
  data.bit(19) = cameraUp->value();
  data.bit(20) = r->value();
  data.bit(21) = l->value();
  data.bit(22) = 0;  //GND
  data.bit(23) = 0;  //RST
  data.bit(24) = right->value() & !left->value();
  data.bit(25) = left->value() & !right->value();
  data.bit(26) = down->value() & !up->value();
  data.bit(27) = up->value() & !down->value();
  data.bit(28) = start->value();
  data.bit(29) = z->value();
  data.bit(30) = b->value();
  data.bit(31) = a->value();
  
  //when L+R+Start are pressed: the X/Y axes are zeroed, RST is set, and Start is cleared
  if(l->value() && r->value() && start->value()) {
    data.byte(0) = 0;  //Y-Axis
    data.byte(1) = 0;  //X-Axis
    data.bit(23) = 1;  //RST
    data.bit(28) = 0;  //Start
  }

  return data;
}

auto Gamepad::getInodeChecksum(u8 bank) -> u8 {
  if (bank < 62) {
    u8 checksum = 0;
    for (i32 i=2; i<0x100; i++)
      checksum += ram.read<Byte>((1 + bank) * 0x100 + i);
    return checksum;
  }

  return 0;
}

//controller paks contain 32KB * nBanks of SRAM split into 128 pages of 256 bytes each.
//the first 3 + nBanks * 2 pages of bank 0 are for storing system data, and the remaining 123 for game data.
//the remaining banks page 0 is unused and the remaining 127 are for game data.
auto Gamepad::formatControllerPak() -> void {
  ram.fill(0x00);

  //page 0 (system area)
  n6  fieldA = random();
  n19 fieldB = random();
  n27 fieldC = random();
  for(u32 area : array<u8[4]>{1,3,4,6}) {
    ram.write<Byte>(area * 0x20 + 0x01, fieldA);                        //unknown
    ram.write<Word>(area * 0x20 + 0x04, fieldB);                        //serial# hi
    ram.write<Word>(area * 0x20 + 0x08, fieldC);                        //serial# lo
    ram.write<Half>(area * 0x20 + 0x18, 0x0001);                        //device ID
    ram.write<Byte>(area * 0x20 + 0x1a, system.controllerPakBankCount); //banks (0x01 = 32KB), (62 = max banks)
    ram.write<Byte>(area * 0x20 + 0x1b, 0x00);                          //version#
    u16 checksum = 0;
    u16 inverted = 0;
    for(u32 half : range(14)) {
      u16 data = ram.read<Half>(area * 0x20 + half * 2);
      checksum +=  data;
      inverted += ~data;
    }
    ram.write<Half>(area * 0x20 + 0x1c, checksum);
    ram.write<Half>(area * 0x20 + 0x1e, inverted);
  }

  //pages 1 thru nBanks, nBanks+1 thru (nBanks*2) (inode table, inode table copy)
  u8 nBanks = ram.read<Byte>(0x20 + 0x1a);
  u32 inodeTablePage = 1;
  u32 inodeTableCopyPage = 1 + nBanks;
  for(u32 bank : range(0,nBanks)) {
    u32 firstDataPage = bank == 0 ? (3 + nBanks * 2) : 1; //first bank has 3 + bank * 2 system pages, other banks have 127.
    for(u32 page : array<u32[2]>{inodeTablePage + bank, inodeTableCopyPage + bank}) {
      for(u32 slot : range(firstDataPage,128)) {
        ram.write<Byte>(0x100 * page + slot * 2 + 0x01, 0x03);  //0x01 = stop, 0x03 = empty
      }
      ram.write<Byte>(0x100 * page + 0x01, getInodeChecksum(bank));  //checksum
    }
  }

  //page 1 is pak info and serial
  //pages 2-nBanks are for the inode table
  //pages at nBanks+1,2*nBanks are for the inode table backup
  //pages at 2*nBanks+1, 2*nBanks+2 are for note table
  //pages 3 + 2*nBanks are for save data
}

auto Gamepad::serialize(serializer& s) -> void {
  s(ram);
  rumble(false);
}
