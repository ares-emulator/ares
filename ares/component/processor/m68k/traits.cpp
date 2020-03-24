template<> auto M68K::bytes<Byte>() -> uint { return 1; }
template<> auto M68K::bytes<Word>() -> uint { return 2; }
template<> auto M68K::bytes<Long>() -> uint { return 4; }

template<> auto M68K::bits<Byte>() -> uint { return  8; }
template<> auto M68K::bits<Word>() -> uint { return 16; }
template<> auto M68K::bits<Long>() -> uint { return 32; }

template<> auto M68K::lsb<Byte>() -> uint32 { return 1; }
template<> auto M68K::lsb<Word>() -> uint32 { return 1; }
template<> auto M68K::lsb<Long>() -> uint32 { return 1; }

template<> auto M68K::msb<Byte>() -> uint32 { return       0x80; }
template<> auto M68K::msb<Word>() -> uint32 { return     0x8000; }
template<> auto M68K::msb<Long>() -> uint32 { return 0x80000000; }

template<> auto M68K::mask<Byte>() -> uint32 { return       0xff; }
template<> auto M68K::mask<Word>() -> uint32 { return     0xffff; }
template<> auto M68K::mask<Long>() -> uint32 { return 0xffffffff; }

template<> auto M68K::clip<Byte>(uint32 data) -> uint32 { return  (uint8)data; }
template<> auto M68K::clip<Word>(uint32 data) -> uint32 { return (uint16)data; }
template<> auto M68K::clip<Long>(uint32 data) -> uint32 { return (uint32)data; }

template<> auto M68K::sign<Byte>(uint32 data) -> int32 { return  (int8)data; }
template<> auto M68K::sign<Word>(uint32 data) -> int32 { return (int16)data; }
template<> auto M68K::sign<Long>(uint32 data) -> int32 { return (int32)data; }
