auto WDC65816::algorithmADC8(n8 data) -> n8 {
  s32 result;

  if(!DF) {
    result = A.l + data + CF;
  } else {
    result = (A.l & 0x0f) + (data & 0x0f) + (CF << 0);
    if(result > 0x09) result += 0x06;
    CF = result > 0x0f;
    result = (A.l & 0xf0) + (data & 0xf0) + (CF << 4) + (result & 0x0f);
  }

  VF = ~(A.l ^ data) & (A.l ^ result) & 0x80;
  if(DF && result > 0x9f) result += 0x60;
  CF = result > 0xff;
  ZF = (n8)result == 0;
  NF = result & 0x80;

  return A.l = result;
}

auto WDC65816::algorithmADC16(n16 data) -> n16 {
  s32 result;

  if(!DF) {
    result = A.w + data + CF;
  } else {
    result = (A.w & 0x000f) + (data & 0x000f) + (CF <<  0);
    if(result > 0x0009) result += 0x0006;
    CF = result > 0x000f;
    result = (A.w & 0x00f0) + (data & 0x00f0) + (CF <<  4) + (result & 0x000f);
    if(result > 0x009f) result += 0x0060;
    CF = result > 0x00ff;
    result = (A.w & 0x0f00) + (data & 0x0f00) + (CF <<  8) + (result & 0x00ff);
    if(result > 0x09ff) result += 0x0600;
    CF = result > 0x0fff;
    result = (A.w & 0xf000) + (data & 0xf000) + (CF << 12) + (result & 0x0fff);
  }

  VF = ~(A.w ^ data) & (A.w ^ result) & 0x8000;
  if(DF && result > 0x9fff) result += 0x6000;
  CF = result > 0xffff;
  ZF = (n16)result == 0;
  NF = result & 0x8000;

  return A.w = result;
}

auto WDC65816::algorithmAND8(n8 data) -> n8 {
  A.l &= data;
  ZF = A.l == 0;
  NF = A.l & 0x80;
  return A.l;
}

auto WDC65816::algorithmAND16(n16 data) -> n16 {
  A.w &= data;
  ZF = A.w == 0;
  NF = A.w & 0x8000;
  return A.w;
}

auto WDC65816::algorithmASL8(n8 data) -> n8 {
  CF = data & 0x80;
  data <<= 1;
  ZF = data == 0;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmASL16(n16 data) -> n16 {
  CF = data & 0x8000;
  data <<= 1;
  ZF = data == 0;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmBIT8(n8 data) -> n8 {
  ZF = (data & A.l) == 0;
  VF = data & 0x40;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmBIT16(n16 data) -> n16 {
  ZF = (data & A.w) == 0;
  VF = data & 0x4000;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmCMP8(n8 data) -> n8 {
  s32 result = A.l - data;
  CF = result >= 0;
  ZF = (n8)result == 0;
  NF = result & 0x80;
  return result;
}

auto WDC65816::algorithmCMP16(n16 data) -> n16 {
  s32 result = A.w - data;
  CF = result >= 0;
  ZF = (n16)result == 0;
  NF = result & 0x8000;
  return result;
}

auto WDC65816::algorithmCPX8(n8 data) -> n8 {
  s32 result = X.l - data;
  CF = result >= 0;
  ZF = (n8)result == 0;
  NF = result & 0x80;
  return result;
}

auto WDC65816::algorithmCPX16(n16 data) -> n16 {
  s32 result = X.w - data;
  CF = result >= 0;
  ZF = (n16)result == 0;
  NF = result & 0x8000;
  return result;
}

auto WDC65816::algorithmCPY8(n8 data) -> n8 {
  s32 result = Y.l - data;
  CF = result >= 0;
  ZF = (n8)result == 0;
  NF = result & 0x80;
  return result;
}

auto WDC65816::algorithmCPY16(n16 data) -> n16 {
  s32 result = Y.w - data;
  CF = result >= 0;
  ZF = (n16)result == 0;
  NF = result & 0x8000;
  return result;
}

auto WDC65816::algorithmDEC8(n8 data) -> n8 {
  data--;
  ZF = data == 0;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmDEC16(n16 data) -> n16 {
  data--;
  ZF = data == 0;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmEOR8(n8 data) -> n8 {
  A.l ^= data;
  ZF = A.l == 0;
  NF = A.l & 0x80;
  return A.l;
}

auto WDC65816::algorithmEOR16(n16 data) -> n16 {
  A.w ^= data;
  ZF = A.w == 0;
  NF = A.w & 0x8000;
  return A.w;
}

auto WDC65816::algorithmINC8(n8 data) -> n8 {
  data++;
  ZF = data == 0;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmINC16(n16 data) -> n16 {
  data++;
  ZF = data == 0;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmLDA8(n8 data) -> n8 {
  A.l = data;
  ZF = A.l == 0;
  NF = A.l & 0x80;
  return data;
}

auto WDC65816::algorithmLDA16(n16 data) -> n16 {
  A.w = data;
  ZF = A.w == 0;
  NF = A.w & 0x8000;
  return data;
}

auto WDC65816::algorithmLDX8(n8 data) -> n8 {
  X.l = data;
  ZF = X.l == 0;
  NF = X.l & 0x80;
  return data;
}

auto WDC65816::algorithmLDX16(n16 data) -> n16 {
  X.w = data;
  ZF = X.w == 0;
  NF = X.w & 0x8000;
  return data;
}

auto WDC65816::algorithmLDY8(n8 data) -> n8 {
  Y.l = data;
  ZF = Y.l == 0;
  NF = Y.l & 0x80;
  return data;
}

auto WDC65816::algorithmLDY16(n16 data) -> n16 {
  Y.w = data;
  ZF = Y.w == 0;
  NF = Y.w & 0x8000;
  return data;
}

auto WDC65816::algorithmLSR8(n8 data) -> n8 {
  CF = data & 1;
  data >>= 1;
  ZF = data == 0;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmLSR16(n16 data) -> n16 {
  CF = data & 1;
  data >>= 1;
  ZF = data == 0;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmORA8(n8 data) -> n8 {
  A.l |= data;
  ZF = A.l == 0;
  NF = A.l & 0x80;
  return A.l;
}

auto WDC65816::algorithmORA16(n16 data) -> n16 {
  A.w |= data;
  ZF = A.w == 0;
  NF = A.w & 0x8000;
  return A.w;
}

auto WDC65816::algorithmROL8(n8 data) -> n8 {
  bool carry = CF;
  CF = data & 0x80;
  data = data << 1 | carry;
  ZF = data == 0;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmROL16(n16 data) -> n16 {
  bool carry = CF;
  CF = data & 0x8000;
  data = data << 1 | carry;
  ZF = data == 0;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmROR8(n8 data) -> n8 {
  bool carry = CF;
  CF = data & 1;
  data = carry << 7 | data >> 1;
  ZF = data == 0;
  NF = data & 0x80;
  return data;
}

auto WDC65816::algorithmROR16(n16 data) -> n16 {
  bool carry = CF;
  CF = data & 1;
  data = carry << 15 | data >> 1;
  ZF = data == 0;
  NF = data & 0x8000;
  return data;
}

auto WDC65816::algorithmSBC8(n8 data) -> n8 {
  s32 result;
  data = ~data;

  if(!DF) {
    result = A.l + data + CF;
  } else {
    result = (A.l & 0x0f) + (data & 0x0f) + (CF << 0);
    if(result <= 0x0f) result -= 0x06;
    CF = result > 0x0f;
    result = (A.l & 0xf0) + (data & 0xf0) + (CF << 4) + (result & 0x0f);
  }

  VF = ~(A.l ^ data) & (A.l ^ result) & 0x80;
  if(DF && result <= 0xff) result -= 0x60;
  CF = result > 0xff;
  ZF = (n8)result == 0;
  NF = result & 0x80;

  return A.l = result;
}

auto WDC65816::algorithmSBC16(n16 data) -> n16 {
  s32 result;
  data = ~data;

  if(!DF) {
    result = A.w + data + CF;
  } else {
    result = (A.w & 0x000f) + (data & 0x000f) + (CF <<  0);
    if(result <= 0x000f) result -= 0x0006;
    CF = result > 0x000f;
    result = (A.w & 0x00f0) + (data & 0x00f0) + (CF <<  4) + (result & 0x000f);
    if(result <= 0x00ff) result -= 0x0060;
    CF = result > 0x00ff;
    result = (A.w & 0x0f00) + (data & 0x0f00) + (CF <<  8) + (result & 0x00ff);
    if(result <= 0x0fff) result -= 0x0600;
    CF = result > 0x0fff;
    result = (A.w & 0xf000) + (data & 0xf000) + (CF << 12) + (result & 0x0fff);
  }

  VF = ~(A.w ^ data) & (A.w ^ result) & 0x8000;
  if(DF && result <= 0xffff) result -= 0x6000;
  CF = result > 0xffff;
  ZF = (n16)result == 0;
  NF = result & 0x8000;

  return A.w = result;
}

auto WDC65816::algorithmTRB8(n8 data) -> n8 {
  ZF = (data & A.l) == 0;
  data &= ~A.l;
  return data;
}

auto WDC65816::algorithmTRB16(n16 data) -> n16 {
  ZF = (data & A.w) == 0;
  data &= ~A.w;
  return data;
}

auto WDC65816::algorithmTSB8(n8 data) -> n8 {
  ZF = (data & A.l) == 0;
  data |= A.l;
  return data;
}

auto WDC65816::algorithmTSB16(n16 data) -> n16 {
  ZF = (data & A.w) == 0;
  data |= A.w;
  return data;
}
