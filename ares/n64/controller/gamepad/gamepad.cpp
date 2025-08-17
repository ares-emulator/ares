#include "transfer-pak.cpp"

Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  port = node->append<Node::Port>("Pak");
  port->setFamily("Nintendo 64");
  port->setType("Pak");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setConnect([&] { return connect(); });
  port->setDisconnect([&] { return disconnect(); });
  port->setSupported({"Controller Pak", "Rumble Pak", "Transfer Pak"});

  bank = 0;

  axis = node->append<Node::Input::Axis>("Axis");

  x                 = node->append<Node::Input::Axis>("X-Axis");
  y                 = node->append<Node::Input::Axis>("Y-Axis");
  up                = node->append<Node::Input::Button>("Up");
  down              = node->append<Node::Input::Button>("Down");
  left              = node->append<Node::Input::Button>("Left");
  right             = node->append<Node::Input::Button>("Right");
  b                 = node->append<Node::Input::Button>("B");
  a                 = node->append<Node::Input::Button>("A");
  cameraUp          = node->append<Node::Input::Button>("C-Up");
  cameraDown        = node->append<Node::Input::Button>("C-Down");
  cameraLeft        = node->append<Node::Input::Button>("C-Left");
  cameraRight       = node->append<Node::Input::Button>("C-Right");
  l                 = node->append<Node::Input::Button>("L");
  r                 = node->append<Node::Input::Button>("R");
  z                 = node->append<Node::Input::Button>("Z");
  start             = node->append<Node::Input::Button>("Start");
  maxOutputReducer1 = node->append<Node::Input::Button>("Max Output Reducer 1");
  maxOutputReducer2 = node->append<Node::Input::Button>("Max Output Reducer 2");
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
  return {};
}

auto Gamepad::connect() -> void {
  if(!slot) return;
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
        fp->read(array_span<u8>{&banks, sizeof(banks)});
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
}

auto Gamepad::disconnect() -> void {
  if(!slot) return;
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
    output[2] = 0x02;  //0x02 = nothing present in controller slot
    if(ram || motor || (slot && slot->name() == "Transfer Pak")) {
      output[2] = 0x01;  //0x01 = pak present
    }
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
    //controller pak
    if(ram) {
      u16 address = (input[1] << 8 | input[2] << 0) & ~31;
      if(pif.addressCRC(address) == (n5)input[2]) {
        for(u32 index : range(recv - 1)) {
          // read into current bank
          if(address <= 0x7FFF) output[index] = ram.read<Byte>(bank * 32_KiB + address);
          else output[index] = 0;
          address++;
        }
        output[recv - 1] = pif.dataCRC({&output[0], recv - 1u});
        valid = 1;
      }
    }

    //rumble pak
    if(motor) {
      u16 address = (input[1] << 8 | input[2] << 0) & ~31;
      if(pif.addressCRC(address) == (n5)input[2]) {
        for(u32 index : range(recv - 1)) {
          if(address <= 0x7FFF) output[index] = 0;
          else if(address <= 0x8FFF) output[index] = 0x80;
          else output[index] = motor->enable() ? 0xFF : 0x00;
          address++;
        }
        output[recv - 1] = pif.dataCRC({&output[0], recv - 1u});
        valid = 1;
      }
    }

    //transfer pak
    if(slot && slot->name() == "Transfer Pak") {
      u16 address = (input[1] << 8 | input[2] << 0) & ~31;
      if(pif.addressCRC(address) == (n5)input[2]) {
        for(u32 index : range(recv - 1)) output[index] = transferPak.read(address++);
        output[recv - 1] = pif.dataCRC({&output[0], recv - 1u});
        valid = 1;
      }
    }
  }

  //write pak
  if(input[0] == 0x03 && send >= 3 && recv >= 1) {
    //controller pak
    if(ram) {
      u16 address = (input[1] << 8 | input[2] << 0) & ~31;
      if(pif.addressCRC(address) == (n5)input[2]) {
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
              debug(unusual, "Controller Pak bank switch command with invalid data length");
            }
          }


          output[0] = pif.dataCRC({&input[3], send - 3u});
          valid = 1;
        } else {
          for(u32 index : range(send - 3)) {
            if(address <= 0x7FFF) ram.write<Byte>(bank * 32_KiB + address, input[3 + index]);
            address++;
          }
          output[0] = pif.dataCRC({&input[3], send - 3u});
          valid = 1;
        }
      }
    }

    //rumble pak
    if(motor) {
      u16 address = (input[1] << 8 | input[2] << 0) & ~31;
      if(pif.addressCRC(address) == (n5)input[2]) {
        output[0] = pif.dataCRC({&input[3], send - 3u});
        valid = 1;
        if(address >= 0xC000) rumble(input[3] & 1);
      }
    }

    //transfer pak
    if(slot && slot->name() == "Transfer Pak") {
      u16 address = (input[1] << 8 | input[2] << 0) & ~31;
      if(pif.addressCRC(address) == (n5)input[2]) {
        for(u32 index : range(send - 3)) {
          transferPak.write(address++, input[3 + index]);
        }
        output[0] = pif.dataCRC({&input[3], send - 3u});
        valid = 1;
      }
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
  platform->input(maxOutputReducer1);
  platform->input(maxOutputReducer2);

  Node::Setting::String stickOutputStyle = axis->append<Node::Setting::String>("Stick Output Style", "Octagon (Virtual) (Default)", [&](auto value) {
    axis->setOutputStyle(value);
  });
  stickOutputStyle->setDynamic(true);
  stickOutputStyle->setAllowedValues({"Custom Octagon (Virtual)", "Custom Circle", "Custom Octagon (Morphed)", "Circle (Diagonal)", "Octagon (Virtual) (Default)", "Circle (Maximum)", "Circle (Cardinal)", "Octagon (Morphed)", "Square (Maximum Virtual)", "Square (Maximum Morphed)"});

  Node::Setting::Real stickMaxOutputReducerOneFactor = axis->append<Node::Setting::Real>("Stick Max Output Reducer 1 Factor", 0.50, [&](auto value) {
    axis->setMaxOutputReducerOneFactor(value);
  });
  stickMaxOutputReducerOneFactor->setDynamic(true);

  Node::Setting::Real stickMaxOutputReducerTwoFactor = axis->append<Node::Setting::Real>("Stick Max Output Reducer 2 Factor", 0.25, [&](auto value) {
    axis->setMaxOutputReducerTwoFactor(value);
  });
  stickMaxOutputReducerTwoFactor->setDynamic(true);

  Node::Setting::Real stickCustomMaxOutput = axis->append<Node::Setting::Real>("Stick Custom Max Output", 85.0, [&](auto value) {
    axis->setCustomMaxOutput(value);
  });
  stickCustomMaxOutput->setDynamic(true);

  Node::Setting::String stickDeadzoneShape = axis->append<Node::Setting::String>("Stick Deadzone Shape", "Axial", [&](auto value) {
    axis->setDeadzoneShape(value);
  });
  stickDeadzoneShape->setDynamic(true);
  stickDeadzoneShape->setAllowedValues({"Axial", "Radial"});

  Node::Setting::Real stickDeadzoneSize = axis->append<Node::Setting::Real>("Stick Deadzone Size", 7.0, [&](auto value) {
    axis->setDeadzoneSize(value);
  });
  stickDeadzoneSize->setDynamic(true);

  Node::Setting::Real stickProportionalSensitivity = axis->append<Node::Setting::Real>("Stick Proportional Sensitivity", 1.0, [&](auto value) {
    axis->setProportionalSensitivity(value);
  });
  stickProportionalSensitivity->setDynamic(true);

  Node::Setting::String stickResponseCurve = axis->append<Node::Setting::String>("Stick Response Curve", "Linear (Default)", [&](auto value) {
    axis->setResponseCurve(value);
  });
  stickResponseCurve->setDynamic(true);
  stickResponseCurve->setAllowedValues({"Linear (Default)", "Relaxed to Aggressive", "Aggressive to Relaxed", "Relaxed to Linear", "Linear to Relaxed", "Aggressive to Linear", "Linear to Aggressive"});


  Node::Setting::Real stickRangeNormalizedSwitchDistance = axis->append<Node::Setting::Real>("Stick Range Normalized Switch Distance", 0.5, [&](auto value) {
    axis->setRangeNormalizedSwitchDistance(value);
  });
  stickRangeNormalizedSwitchDistance->setDynamic(true);

  Node::Setting::Real stickResponseStrength = axis->append<Node::Setting::Real>("Stick Response Strength", 0.0, [&](auto value) {
    axis->setResponseStrength(value);
  });
  stickResponseStrength->setDynamic(true);

  Node::Setting::Boolean stickVirtualNotch = axis->append<Node::Setting::Boolean>("Enable Stick Virtual Notches", false, [&](auto value) {
    axis->virtualNotch();
  });
  stickVirtualNotch->setDynamic(true);

  Node::Setting::Real stickNotchLengthFromEdge = axis->append<Node::Setting::Real>("Stick Notch Length from Edge", 0.1, [&](auto value) {
    axis->setNotchLengthFromEdge(value);
  });
  stickNotchLengthFromEdge->setDynamic(true);

  Node::Setting::Real stickNotchAngularSnappingDistance = axis->append<Node::Setting::Real>("Stick Notch Angular Snapping Distance", 0.0, [&](auto value) {
    axis->setNotchAngularSnappingDistance(value);
  });
  stickNotchAngularSnappingDistance->setDynamic(true);

  string configuredOutputStyleChoice = axis->outputStyle();
  if(configuredOutputStyleChoice == "Custom Octagon (Virtual)") outputStyle = OutputStyle::CustomVirtualOctagon;
  if(configuredOutputStyleChoice == "Custom Circle") outputStyle = OutputStyle::CustomCircle;
  if(configuredOutputStyleChoice == "Custom Octagon (Morphed)") outputStyle = OutputStyle::CustomMorphedOctagon;
  if(configuredOutputStyleChoice == "Circle (Diagonal)") outputStyle = OutputStyle::DiagonalCircle;
  if(configuredOutputStyleChoice == "Octagon (Virtual) (Default)") outputStyle = OutputStyle::VirtualOctagon;
  if(configuredOutputStyleChoice == "Circle (Maximum)") outputStyle = OutputStyle::MaxCircle;
  if(configuredOutputStyleChoice == "Circle (Cardinal)") outputStyle = OutputStyle::CardinalCircle;
  if(configuredOutputStyleChoice == "Octagon (Morphed)") outputStyle = OutputStyle::MorphedOctagon;
  if(configuredOutputStyleChoice == "Square (Maximum Virtual)") outputStyle = OutputStyle::MaxVirtualSquare;
  if(configuredOutputStyleChoice == "Square (Maximum Morphed)") outputStyle = OutputStyle::MaxMorphedSquare;

  auto cardinalMax   = 85.0;
  auto diagonalMax   = 69.0;
  auto innerDeadzone =  7.0; // default should remain 7 (~8.2% of 85) as the deadzone is axial in nature and fights cardinalMax
  auto saturationRadius = (innerDeadzone + diagonalMax + sqrt(pow(innerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * innerDeadzone)) / sqrt(2.0); //from linear response curve function within axis->processDeadzoneAndResponseCurve, substitute saturationRadius * sqrt(2) / 2 for right-hand lengthAbsolute and set diagonalMax as the result then solve for saturationRadius
  auto offset = 0.0;

  string configuredDeadzoneShape = axis->deadzoneShape(); //user-defined Axial or Radial
  auto configuredInnerDeadzone = max(0.0, axis->deadzoneSize()); //user-defined [0, cardinalMax) (default 7.0); deadzone where input less than assigned value is 0
  auto configuredCustomMaxOutput = max(0.0, axis->customMaxOutput()); //user-defined [0, 127] (default 85.0); only affects outputStyleChoice strings that include "Custom"
  auto customMaxOutputMultiplier = configuredCustomMaxOutput / cardinalMax; //for CustomOctagon, customMaxOutput divided by its saturationRadius could be done instead but the current works fine because of the later clamps
  
  auto maxOutputReduction1 = (maxOutputReducer1->value()) ? max(0.0, axis->maxOutputReducerOneFactor()) : 0.0; //user-defined (default 0.50); same questions as above; currently tied to either a button or key hold for activation; Is this best, or is switching to a toggle, cycle, or some combination better?
  auto maxOutputReduction2 = (maxOutputReducer2->value()) ? max(0.0, axis->maxOutputReducerTwoFactor()) : 0.0; //user-defined (default 0.25); refer to nearest above comment
  auto maxOutputReduction = max(0.0, 1.0 - (maxOutputReduction1 + maxOutputReduction2));

  switch(outputStyle) { //too messy? single-line cases seemed worse and if-else statements seemed hard to follow

  case OutputStyle::CustomMorphedOctagon:
    diagonalMax = maxOutputReduction * customMaxOutputMultiplier * 69.0;
    cardinalMax = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    saturationRadius = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    break;
  case OutputStyle::CustomVirtualOctagon:
    diagonalMax = maxOutputReduction * customMaxOutputMultiplier * 69.0;
    cardinalMax = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    break;
  case OutputStyle::CustomCircle:
    cardinalMax = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    saturationRadius = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    break;
  case OutputStyle::VirtualOctagon:
    diagonalMax = maxOutputReduction * 69.0;
    cardinalMax = maxOutputReduction * 85.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    break;
  case OutputStyle::DiagonalCircle:
    diagonalMax = maxOutputReduction * 69.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    cardinalMax = saturationRadius;
    break;
  case OutputStyle::MaxCircle:
    cardinalMax = maxOutputReduction * 127.0;
    saturationRadius = maxOutputReduction * 127.0;
    break;
  case OutputStyle::CardinalCircle:
    cardinalMax = maxOutputReduction * 85.0;
    saturationRadius = maxOutputReduction * 85.0;
    break;
  case OutputStyle::MorphedOctagon:
    diagonalMax = maxOutputReduction * 69.0;
    cardinalMax = maxOutputReduction * 85.0;
    saturationRadius = maxOutputReduction * 85.0;
    break;
  case OutputStyle::MaxVirtualSquare:
    diagonalMax = maxOutputReduction * 127.0;
    cardinalMax = maxOutputReduction * 127.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    break;
  case OutputStyle::MaxMorphedSquare:
    diagonalMax = maxOutputReduction * 127.0;
    cardinalMax = maxOutputReduction * 127.0;
    saturationRadius = maxOutputReduction * 127.0;
    break;
  }

  //scale {-32767 ... +32767} to {-saturationRadius + offset ... +saturationRadius + offset}
  auto ax = axis->setOperatingRange(x->value(), saturationRadius, offset);
  auto ay = axis->setOperatingRange(y->value(), saturationRadius, offset);

  auto configuredNotchLengthFromEdge = axis->notchLengthFromEdge(); //user-defined [0.0, 1.0] (default 0.1); the default value cannot be configured by user in other projects
  auto configuredMaxNotchAngularDist = axis->notchAngularSnappingDistance(); //user-defined in degrees [0.0, 45.0] (default 0.0); values under 15.0 are likely to be more favorable
  auto configuredVirtualNotch = axis->virtualNotch();
  if(configuredVirtualNotch == true) {
    auto initialLength = hypot(ax - offset, ay - offset);
    auto initialAngle = atan2(ay - offset, ax - offset);
    auto currentAngle = axis->virtualNotch(initialLength, initialAngle, saturationRadius, configuredNotchLengthFromEdge, configuredMaxNotchAngularDist);
    ax = cos(currentAngle) * initialLength + offset;
    ay = sin(currentAngle) * initialLength + offset;
  }

  string configuredResponseCurveMode = axis->responseCurve();
  if(configuredResponseCurveMode == "Linear (Default)") response = Response::Linear;
  if(configuredResponseCurveMode == "Relaxed to Aggressive") response = Response::RelaxedToAggressive;
  if(configuredResponseCurveMode == "Aggressive to Relaxed") response = Response::AggressiveToRelaxed;
  if(configuredResponseCurveMode == "Relaxed to Linear") response = Response::RelaxedToLinear;
  if(configuredResponseCurveMode == "Linear to Relaxed") response = Response::LinearToRelaxed;
  if(configuredResponseCurveMode == "Aggressive to Linear") response = Response::AggressiveToLinear;
  if(configuredResponseCurveMode == "Linear to Aggressive") response = Response::LinearToAggressive;

  auto configuredRangeNormalizedSwitchDistance = std::clamp(axis->rangeNormalizedSwitchDistance(), 0.001, 0.999); //user-defined (configuredInnerDeadzone, cardinalMax) (default 50.0)
  auto configuredResponseStrength = axis->responseStrength(); //user-defined (0.0, 100.0] (default 100.0); used to produce a more relaxed or aggressive curve; values close to 0.0 and 100.0 create strongest response when relaxed and aggressive, respectively
  auto configuredProportionalSensitivity = axis->proportionalSensitivity(); //user-defined (default 1.0); Should this only apply to a linear response? What percentage range should be used? Place outside of Gamepad::responseCurve()?

  //create inner axial dead-zone in range {-configuredInnerDeadzone ... +configuredInnerDeadzone} and scale from it up to saturationRadius
  if(configuredDeadzoneShape == "Axial"){
    auto lengthAbsoluteX = abs(ax - offset);
    auto lengthAbsoluteY = abs(ay - offset);
    ax = axis->processDeadzoneAndResponseCurve(ax, lengthAbsoluteX, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedSwitchDistance, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
    ay = axis->processDeadzoneAndResponseCurve(ay, lengthAbsoluteY, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedSwitchDistance, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
  } else {
    auto length = hypot(ax - offset, ay - offset);
    ax = axis->processDeadzoneAndResponseCurve(ax, length, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedSwitchDistance, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
    ay = axis->processDeadzoneAndResponseCurve(ay, length, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedSwitchDistance, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
  }

  auto scaledLength = hypot(ax - offset, ay - offset);
  if(scaledLength > saturationRadius) {
    scaledLength = saturationRadius / scaledLength;
    ax = axis->revisePosition(ax, scaledLength, offset);
    ay = axis->revisePosition(ay, scaledLength, offset);
  }

  //let cardinalMax and diagonalMax define boundaries and restrict to an octagonal gate
  if(outputStyle == OutputStyle::VirtualOctagon || outputStyle == OutputStyle::CustomVirtualOctagon || outputStyle == OutputStyle::MorphedOctagon || outputStyle == OutputStyle::CustomMorphedOctagon || outputStyle == OutputStyle::MaxVirtualSquare || outputStyle == OutputStyle::MaxMorphedSquare) {
    if(ax != offset && ay != offset) {
      double edgex = 0.0;
      double edgey = 0.0;

      axis->applyGateBoundaries(cardinalMax, diagonalMax, ax, ay, offset, edgex, edgey);

      auto distanceToEdge = hypot(edgex, edgey);

      if(outputStyle == OutputStyle::VirtualOctagon || outputStyle == OutputStyle::CustomVirtualOctagon || outputStyle == OutputStyle::MaxVirtualSquare) {
        auto currentLength = hypot(ax - offset, ay - offset);
        if(currentLength > distanceToEdge) {
          ax = edgex;
          ay = edgey;
        }
      } else {
        auto scale = distanceToEdge / saturationRadius;
        ax = axis->revisePosition(ax, scale, offset);
        ay = axis->revisePosition(ay, scale, offset);
      }
    }
  }

  //keep cardinal input within positive and negative bounds of cardinalMax
  if(outputStyle == OutputStyle::VirtualOctagon || outputStyle == OutputStyle::CustomVirtualOctagon || outputStyle == OutputStyle::MaxVirtualSquare) {
    ax = axis->clampAxisToNearestBoundary(ax, offset, cardinalMax);
    ay = axis->clampAxisToNearestBoundary(ay, offset, cardinalMax);
  }

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
