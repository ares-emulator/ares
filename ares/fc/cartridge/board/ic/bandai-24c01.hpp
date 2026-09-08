#pragma once

//LSB-first 24C01 serial interface used by Bandai Datach cartridges.
struct Bandai24C01 {
  auto power() -> void {
    mode = Mode::Standby;
    clock = data = response = 1;
    counter = address = input = output = reading = pending = 0;
    for(auto& byte : page) byte = 0;
  }
  auto read() const -> bool {
    return response;
  }
  auto write(bool scl, bool sda) -> void {
    if(clock && scl && data && !sda) {
      mode = Mode::Address;
      counter = address = pending = 0;
      response = 1;
    } else if(clock && scl && !data && sda) {
      //The four-byte page buffer is programmed on STOP. Programming is instantaneous.
      for(u32 index : range(4)) {
        if(pending.bit(index)) memory[(address & 0x7c) | index] = page[index];
      }
      pending = 0;
      mode = Mode::Standby;
      response = 1;
    } else if(!clock && scl) {
      switch(mode) {
      case Mode::Address:
        if(counter < 7) address.bit(counter++) = sda;
        else if(counter == 7) reading = sda, counter++;
        break;
      case Mode::Read:
        if(counter < 8) counter++;
        break;
      case Mode::ReadAck:
        reading = !sda;
        break;
      case Mode::Write:
        if(counter < 8) input.bit(counter++) = sda;
        break;
      default:
        break;
      }
    } else if(clock && !scl) {
      switch(mode) {
      case Mode::Address:
        if(counter == 8) mode = Mode::AddressAck, response = 0;
        break;
      case Mode::AddressAck:
        counter = 0;
        mode = reading ? Mode::Read : Mode::Write;
        output = memory[address];
        response = reading ? output.bit(0) : 1;
        break;
      case Mode::Read:
        if(counter == 8) {
          mode = Mode::ReadAck;
          response = 1;
        } else {
          response = output.bit(counter);
        }
        break;
      case Mode::ReadAck:
        if(reading) {
          address++;
          counter = 0;
          output = memory[address];
          response = output.bit(0);
          mode = Mode::Read;
        } else {
          mode = Mode::Standby;
          response = 1;
        }
        break;
      case Mode::Write:
        if(counter == 8) {
          page[address.bit(0,1)] = input;
          pending.bit(address.bit(0,1)) = 1;
          address.bit(0,1)++;
          mode = Mode::WriteAck;
          response = 0;
        }
        break;
      case Mode::WriteAck:
        mode = Mode::Write;
        counter = 0;
        response = 1;
        break;
      default:
        break;
      }
    }
    clock = scl;
    data = sda;
  }
  auto serialize(serializer& s) -> void {
    s(memory);
    s(clock);
    s(data);
    s((u32&)mode);
    s(counter);
    s(address);
    s(input);
    s(output);
    s(reading);
    s(response);
    s(page);
    s(pending);
  }

  n8 memory[128];
  n1 clock = 1;
  n1 data = 1;

private:
  enum class Mode : u32 { Standby, Address, AddressAck, Read, ReadAck, Write, WriteAck };
  Mode mode = Mode::Standby;
  n4 counter;
  n7 address;
  n8 input;
  n8 output;
  n1 reading;
  n1 response = 1;
  n8 page[4];
  n4 pending;
};
