auto CPU::getControlRegister(u8 index) -> u32 {
  uint32 data = 0;

  switch(index & 15) {

  case  0:  //Index
    break;

  case  1:  //Random
    break;

  case  2:  //BusControl
//case  2:  //EntryLo
    break;

  case  3:  //Config
    break;

  case  4:  //Context
    break;

  case  8:  //BadVirtualAddress
    break;

  case  9:  //Count
    break;

  case 10:  //PortSize
//case 10:  //EntryHi
    break;

  case 11:  //Compare
    break;

  case 12:  //StatusRegister
    break;

  case 13:  //Cause
    break;

  case 14:  //EPC
    break;

  case 15:  //ProductID
    data.bit(0, 7) = 0x00;  //revision (unverified)
    data.bit(8,15) = 0x03;  //implementation (unverified)
    break;

  }

  return data;
}

auto CPU::setControlRegister(u8 index, u32 value) -> void {
  uint32 data = value;

  switch(index & 15) {

  case  0:  //Index
    break;

  case  1:  //Random
    break;

  case  2:  //BusControl
//case  2:  //EntryLo
    break;

  case  3:  //Config
    break;

  case  4:  //Context
    break;

  case  8:  //BadVirtualAddress
    break;

  case  9:  //Count
    break;

  case 10:  //PortSize
//case 10:  //EntryHi
    break;

  case 11:  //Compare
    break;

  case 12:  //StatusRegister
    break;

  case 13:  //Cause
    break;

  case 14:  //EPC
    break;

  case 15:  //ProductID (read-only)
    break;

  }
}
