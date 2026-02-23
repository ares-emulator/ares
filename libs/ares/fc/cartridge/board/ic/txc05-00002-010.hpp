#pragma once
//                                                  TXC 05-00002-010
//                                                        .--\/--.
//                                                  Q2 <- |01  24| -> Q3
//                                                  Q1 <- |02  23| -> Q4
//                                                  Q0 <- |03  22| -> o3
//                                                  i1 -> |04  21| <- CPU A13 (rn)
//                                                  i0 -> |05  20| <- CPU A14 (rn)
//                                                 io2 <> |06  19| -- GND
//                                                  5V -- |07  18| <- CPU R/W (n)
//                                                  D5 <> |08  17| <- /ROMSEL (rn)
//                                                  D4 <> |09  16| <- M2 (n)
//                                                  D2 <> |10  15| <- CPU A8 (rn)
//                                                  D1 <> |11  14| <- CPU A1 (rn)
//                                                  D0 <> |12  13| <- CPU A0 (rn)
//                                                        '------'
//                   36                                     132                                       173
//                .--\/--.                                .--\/--.                                  .--\/--.
//          NC <- |01  24| -> NC           (r) PRG A15 <- |01  24| -> NC                      NC <- |01  24| -> NC
// (r) PRG A16 <- |02  23| -> NC           (r) CHR A14 <- |02  23| -> NC            *(r) CHR A15 <- |02  23| -> NC
// (r) PRG A15 <- |03  22| -> NC           (r) CHR A13 <- |03  22| -> NC             (r) CHR A13 <- |03  22| -> CHR A14
//         GND -> |04  21| <- CPU A13 (rn)         GND -> |04  21| <- CPU A13 (fr)           GND -> |04  21| <- CPU A13 (fr)
//          5V -> |05  20| <- CPU A14 (rn)          5V -> |05  20| <- CPU A14 (fr)            5V -> |05  20| <- CPU A14 (fr)
//          NC <> |06  19| -- GND                   NC <> |06  19| -- GND                     NC <> |06  19| <- GND
//          5V -- |07  18| <- CPU R/W (n)           5V -- |07  18| <- CPU R/W (f)             5V -- |07  18| <- CPU R/W (f)
//          NC <> |08  17| <- /ROMSEL (rn)         GND <> |08  17| <- /ROMSEL (fr)           GND <> |08  17| <- /ROMSEL (fr)
//          NC <> |09  16| <- M2 (n)       (fr) CPU D3 <> |09  16| <- M2 (f)         (fr) CPU D3 <> |09  16| <- M2 (f)
//          NC <> |10  15| <- CPU A8 (rn)  (fr) CPU D2 <> |10  15| <- CPU A8 (fr)    (fr) CPU D2 <> |10  15| <- CPU A8 (fr)
// (rn) CPU D5 <> |11  14| <- CPU A1 (rn)  (fr) CPU D1 <> |11  14| <- CPU A1 (fr)    (fr) CPU D1 <> |11  14| <- CPU A1 (fr)
// (rn) CPU D4 <> |12  13| <- CPU A0 (rn)  (fr) CPU D0 <> |12  13| <- CPU A0 (fr)    (fr) CPU D0 <> |12  13| <- CPU A0 (fr)
//                '------'`                               '------'                                  '------'
// 173*: TXC pin 2 was routed to CHR ROM pin 1 on a 28-pin package.
//       On a UVEPROM, this is A15.
//       However, no games were ever released using more than 32 KiB of CHR.
// TXC 05-00002-010: 36/132/173
struct TXC05_00002_010 {
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
                 if (overdown) {
                   o3 = (invert ? io2 : i0) | data.bit(5);
                 } else {
                   io2 = invert ? i1 : i0;
                   o3 = io2 | data.bit(5);
                 }
                 return;
    case 0x4102: reg.bit(4,5) = data.bit(4,5);
                 in.bit(0,2) = data.bit(0,2);
                 in.bit(3) = in.bit(3) ^ invert;
                 return;
    case 0x4103: increment = data.bit(0);
                 return;
    default:     out.bit(0,3) = reg.bit(0,3);
                 out.bit(4) = reg.bit(4) ^ invert;
                 return;
    }
  }

  auto serialize(serializer& s) -> void {
    s(i0);
    s(i1); 
    s(io2);
    s(o3);

    s(increment);
    s(invert);
    s(overdown);
    s(reg);
    s(in);
    s(out);
  }

  bool overdown; // io2 is using input or output flag

  // 4 pin
  n1 i0, i1, io2, o3;

  n1 increment;
  n1 invert;
  n6 reg;
  n4 in;
  n5 out;
};

