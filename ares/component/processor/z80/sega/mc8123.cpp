// MC-8123 decryption algorithm derived from MAME (BSD-3-Clause).
// See the MAME section in LICENSE.
auto MC8123::reorder(n8 source, const u8 (&bits)[8]) const -> n8 {
  n8 result = 0;
  for(u32 index : range(8)) result.bit(7 - index) = source.bit(bits[index]);
  return result;
}

auto MC8123::permute(n8 source, n3 type, n2 permutation) const -> n8 {
  static constexpr u8 positions[7][4][8] = {
    {
      {7,5,3,1,2,0,6,4},
      {5,3,7,2,1,0,4,6},
      {0,3,4,6,7,1,5,2},
      {0,7,3,2,6,4,1,5},
    },
    {  // Type1a
      {4,2,6,5,3,7,1,0},
      {6,0,5,4,3,2,1,7},
      {2,3,6,1,4,0,7,5},
      {6,5,1,3,2,7,0,4},
    },
    {  // Type1b
      {1,0,3,2,5,6,4,7},
      {2,0,5,1,7,4,6,3},
      {6,4,7,2,0,5,1,3},
      {7,1,3,6,0,2,5,4},
    },
    {  // Type2a
      {0,1,4,3,5,6,2,7},
      {6,3,0,5,7,4,1,2},
      {1,6,4,5,0,3,7,2},
      {4,6,7,5,2,3,1,0},
    },
    {  // Type2b
      {1,3,4,6,5,7,0,2},
      {0,1,5,4,7,3,2,6},
      {3,5,4,1,6,2,0,7},
      {5,2,3,0,4,7,6,1},
    },
    {  // Type3a
      {5,3,1,7,0,2,6,4},
      {3,1,2,5,4,7,0,6},
      {5,6,1,2,7,0,4,3},
      {5,6,7,0,4,2,1,3},
    },
    {  // Type3b
      {3,7,5,4,0,6,2,1},
      {7,5,4,6,1,2,0,3},
      {7,4,3,0,5,1,6,2},
      {2,6,4,1,3,7,0,5},
    },
  };

  return reorder(source, positions[type][permutation]);
}

auto MC8123::type0(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 0, permutation);

  if(parameter.bit(3) && value.bit(7)) value ^= 0x29;

  if(parameter.bit(2) && value.bit(6)) value ^= 0x86;

  if(value.bit(6)) value ^= 0x80;

  if(parameter.bit(1) && value.bit(7)) value ^= 0x40;

  if(value.bit(2)) value ^= 0x21;

  value ^= 0x1a;

  if(parameter.bit(2)) value ^= 0x25;
  if(parameter.bit(1)) value ^= 0xc0;
  if(parameter.bit(0)) value ^= 0x21;

  if(parameter.bit(0)) value = reorder(value, {7,6,5,1,4,3,2,0});

  return value;
}


auto MC8123::type1a(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 1, permutation);

  if(parameter.bit(2)) value = reorder(value, {7,6,1,5,3,2,4,0});

  if(value.bit(1)) value ^= 0x01;
  if(value.bit(6)) value ^= 0x08;
  if(value.bit(7)) value ^= 0x48;
  if(value.bit(2)) value ^= 0x4a;
  if(value.bit(4)) value ^= 0xc4;

  if(value.bit(7) ^ value.bit(2)) value ^= 0x10;

  value ^= 0x4b;

  if(parameter.bit(3)) value ^= 0x84;
  if(parameter.bit(1)) value ^= 0x48;

  if(parameter.bit(0)) value = reorder(value, {7,6,1,4,3,2,5,0});

  return value;
}

auto MC8123::type1b(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 2, permutation);

  if(value.bit(2) && value.bit(0)) value ^= 0x90;

  if(value.bit(7)) value ^= 0x04;
  if(value.bit(5)) value ^= 0x84;
  if(value.bit(1)) value ^= 0x20;
  if(value.bit(6)) value ^= 0x02;
  if(value.bit(4)) value ^= 0x60;
  if(value.bit(0)) value ^= 0x46;
  if(value.bit(3)) value ^= 0xc7;

  value ^= 0x51;

  if(parameter.bit(3)) value ^= 0x12;
  if(parameter.bit(2)) value ^= 0xc9;
  if(parameter.bit(1)) value ^= 0x18;
  if(parameter.bit(0)) value ^= 0x47;

  return value;
}

auto MC8123::type2a(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 3, permutation);

  if(value.bit(3) || (parameter.bit(1) && value.bit(2)))
    value = reorder(value, {6,0,7,4,3,2,1,5});

  if(value.bit(5)) value ^= 0x80;
  if(value.bit(6)) value ^= 0x20;
  if(value.bit(0)) value ^= 0x40;
  if(value.bit(4)) value ^= 0x09;
  if(value.bit(1)) value ^= 0x04;

  value ^= 0xf2;

  if(parameter.bit(2)) value ^= 0x1f;

  if(parameter.bit(3)) {
    if(parameter.bit(0)) value = reorder(value, {7,6,5,3,4,1,2,0});
    else value = reorder(value, {7,6,5,1,2,4,3,0});
  } else if(parameter.bit(0)) {
    value = reorder(value, {7,6,5,2,1,3,4,0});
  }

  return value;
}

auto MC8123::type2b(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 4, permutation);

  if(value.bit(7) && value.bit(3)) value ^= 0x51;

  if(value.bit(7)) value ^= 0x04;
  if(value.bit(5)) value ^= 0x88;
  if(value.bit(1)) value ^= 0x20;
  if(value.bit(4)) value ^= 0xaa;

  if(value.bit(7) && value.bit(5)) value ^= 0x11;

  if(value.bit(5) && value.bit(1)) value ^= 0x11;

  if(value.bit(6)) value ^= 0xa0;
  if(value.bit(3)) value ^= 0xe2;
  if(value.bit(2)) value ^= 0x0a;

  value ^= 0x8e;

  if(parameter.bit(3)) value ^= 0x4a;
  if(parameter.bit(2)) value ^= 0xee;
  if(parameter.bit(1)) value ^= 0x80;
  if(parameter.bit(0)) value ^= 0x24;

  return value;
}

auto MC8123::type3a(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 5, permutation);

  if(value.bit(2)) value ^= 0xb0;
  if(value.bit(3)) value ^= 0x01;

  if(parameter.bit(0)) value = reorder(value, {7,2,5,4,3,1,0,6});

  if(value.bit(1)) value ^= 0x41;
  if(value.bit(3)) value ^= 0x16;

  if(parameter.bit(3)) value ^= 0x18;

  if(value.bit(3)) value = reorder(value, {5,6,7,4,3,2,1,0});

  if(value.bit(5)) value ^= 0x06;

  value ^= 0x78;

  if(parameter.bit(2)) value ^= 0x80;
  if(parameter.bit(1)) value ^= 0x10;
  if(parameter.bit(0)) value ^= 0x01;

  return value;
}

auto MC8123::type3b(n8 value, n4 parameter, n2 permutation) const -> n8 {
  value = permute(value, 6, permutation);

  if(value.bit(2)) value ^= 0x80;

  if(value.bit(7)) value = reorder(value, {7,6,3,4,5,2,1,0});

  if(parameter.bit(3)) value ^= 0x80;

  if(value.bit(4)) value ^= 0x40;
  if(value.bit(1)) value ^= 0x54;

  if(value.bit(7) && value.bit(6)) value ^= 0x02;

  if(value.bit(7)) value ^= 0x02;

  if(parameter.bit(3)) value ^= 0x80;
  if(parameter.bit(2)) value ^= 0x01;

  if(parameter.bit(3)) value = reorder(value, {4,6,3,2,5,0,1,7});

  if(value.bit(4)) value ^= 0x02;
  if(value.bit(5)) value ^= 0x10;
  if(value.bit(7)) value ^= 0x04;

  value ^= 0x2c;

  if(parameter.bit(1)) value ^= 0x80;
  if(parameter.bit(0)) value ^= 0x08;

  return value;
}

auto MC8123::transform(n8 value, n8 key, bool opcode) const -> n8 {
  key ^= 0xff;
  if(!key) return value;

  n3 type = 0;
  type.bit(0) = key.bit(0) ^ key.bit(2);
  type.bit(1) = key.bit(0) ^ key.bit(1) ^ key.bit(2) ^ key.bit(4);
  type.bit(2) = key.bit(4) ^ key.bit(5);

  n2 permutation = 0;
  permutation.bit(0) = key.bit(0) ^ key.bit(1);
  permutation.bit(1) = key.bit(2) ^ key.bit(3);

  n4 parameter = 0;
  parameter.bit(0) = key.bit(0);
  parameter.bit(1) = key.bit(0) ^ key.bit(2) ^ key.bit(3);
  parameter.bit(2) = key.bit(0) ^ key.bit(1) ^ key.bit(6);
  parameter.bit(3) = key.bit(1) ^ key.bit(6) ^ key.bit(7);

  if(!opcode) {
    parameter.bit(0) = !parameter.bit(0);
    type.bit(0) = !type.bit(0);
  }

  switch(type) {
  case 0:
  case 1: return type0(value, parameter, permutation);
  case 2: return type1a(value, parameter, permutation);
  case 3: return type1b(value, parameter, permutation);
  case 4: return type2a(value, parameter, permutation);
  case 5: return type2b(value, parameter, permutation);
  case 6: return type3a(value, parameter, permutation);
  case 7: return type3b(value, parameter, permutation);
  }
  return value;
}

auto MC8123::decrypt(n16 address, n8 source, bool opcode) const -> n8 {
  if(address >= encryptedSize) return source;
  n12 index = 0;
  index.bit(0) = address.bit(0);
  index.bit(1) = address.bit(1);
  index.bit(2) = address.bit(2);
  index.bit(3) = address.bit(4);
  index.bit(4) = address.bit(6);
  index.bit(5) = address.bit(8);
  index.bit(6) = address.bit(10);
  index.bit(7) = address.bit(11);
  index.bit(8) = address.bit(12);
  index.bit(9) = address.bit(13);
  index.bit(10) = address.bit(14);
  index.bit(11) = address.bit(15);
  return transform(source, keyData[index | (opcode ? 0 : 0x1000)], opcode);
}
