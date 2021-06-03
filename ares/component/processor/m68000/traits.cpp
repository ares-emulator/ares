template<> auto M68000::bytes<Byte>() -> u32 { return 1; }
template<> auto M68000::bytes<Word>() -> u32 { return 2; }
template<> auto M68000::bytes<Long>() -> u32 { return 4; }

template<> auto M68000::bits<Byte>() -> u32 { return  8; }
template<> auto M68000::bits<Word>() -> u32 { return 16; }
template<> auto M68000::bits<Long>() -> u32 { return 32; }

template<> auto M68000::lsb<Byte>() -> n32 { return 1; }
template<> auto M68000::lsb<Word>() -> n32 { return 1; }
template<> auto M68000::lsb<Long>() -> n32 { return 1; }

template<> auto M68000::msb<Byte>() -> n32 { return       0x80; }
template<> auto M68000::msb<Word>() -> n32 { return     0x8000; }
template<> auto M68000::msb<Long>() -> n32 { return 0x80000000; }

template<> auto M68000::mask<Byte>() -> n32 { return       0xff; }
template<> auto M68000::mask<Word>() -> n32 { return     0xffff; }
template<> auto M68000::mask<Long>() -> n32 { return 0xffffffff; }

template<> auto M68000::clip<Byte>(n32 data) -> n32 { return (n8 )data; }
template<> auto M68000::clip<Word>(n32 data) -> n32 { return (n16)data; }
template<> auto M68000::clip<Long>(n32 data) -> n32 { return (n32)data; }

template<> auto M68000::sign<Byte>(n32 data) -> i32 { return (i8 )data; }
template<> auto M68000::sign<Word>(n32 data) -> i32 { return (i16)data; }
template<> auto M68000::sign<Long>(n32 data) -> i32 { return (i32)data; }
