#pragma once
//     JV001: 136/172                             136                              172
//             .--\/--.            |            .--\/--.            |            .--\/--.
//  PPU A13 -> |01  28| -- GND     | PPU A13 -> |01  28| -- GND     | PPU A13 -> |01  28| -- GND
//   CPU D5 <> |02  27| <- PPU R/W |  CPU D5 <> |02  27| <- NC      |  CPU D0 <> |02  27| <- PPU R/W
//   CPU D4 <> |03  26| -> Invert  |  CPU D4 <> |03  26| -> NC      |  CPU D1 <> |03  26| -> CIRAM A10
//   CPU D3 <> |04  25| <- PPU A11 |  CPU D3 <> |04  25| <- NC      |  CPU D2 <> |04  25| <- PPU A11
//   CPU D2 <> |05  24| <- PPU A10 |  CPU D2 <> |05  24| <- NC      |  CPU D3 <> |05  24| <- PPU A10
//   CPU D1 <> |06  23| -> O0      |  CPU D1 <> |06  23| -> NC      |  CPU D4 <> |06  23| -> CHR A13
//   CPU D0 <> |07  22| -> O1      |  CPU D0 <> |07  22| -> NC      |  CPU D5 <> |07  22| -> CHR A14
//   CPU A0 -> |08  21| -> O2      |  CPU A0 -> |08  21| -> NC      |  CPU A0 -> |08  21| -> NC
//   CPU A1 -> |09  20| -> O3      |  CPU A1 -> |09  20| -> NC      |  CPU A1 -> |09  20| -> NC
//   CPU A8 -> |10  19| -> O4      |  CPU A8 -> |10  19| -> NC      |  CPU A8 -> |10  19| -> NC
//       M2 -> |11  18| -> O5      |      M2 -> |11  18| -> NC      |      M2 -> |11  18| -> NC
//  /ROMSEL -> |12  17| <- CHR /OE | /ROMSEL -> |12  17| <- CHR /OE | /ROMSEL -> |12  17| <- CHR /OE
//  CPU R/W -> |13  16| <- CPU A13 | CPU R/W -> |13  16| <- NC      | CPU R/W -> |13  16| <- CPU A13
//       5V -- |14  15| <- CPU A14 |      5V -- |14  15| <- NC      |      5V -- |14  15| <- CPU A14
//             '------'            |            '------'            |            '------'
struct JV001 {
  auto writePRG(n16 address, n8 data) -> void {
    address &= 0xe103;
    switch (address) {
    case 0x4100: if (increment) {
                   ++reg.bit(0,3);
                 } else {
                   reg.bit(0,3) = in ^ (invert ? 0x0f : 0);
                 }
                 return;
    case 0x4101: invert = data.bit(0);
                 return;
    case 0x4102: reg.bit(4,5) = data.bit(4,5);
                 in = data.bit(0,3);
                 return;
    case 0x4103: increment = data.bit(0);
                 return;
    default:     out.bit(0,3) = reg.bit(0,3);
                 out.bit(4,5) = reg.bit(4,5) ^ (invert ? 0x03 : 0);
                 return;
    }
  }

  auto serialize(serializer& s) -> void {
    s(reg);
    s(invert);
    s(in);
    s(increment);
    s(out);
  }

  n6 reg;
  n1 invert;
  n4 in;
  n1 increment;
  n6 out;
};
