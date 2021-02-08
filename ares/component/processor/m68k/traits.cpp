template<> auto M68K::bytes<Byte>() -> u32 { return 1; }
template<> auto M68K::bytes<Word>() -> u32 { return 2; }
template<> auto M68K::bytes<Long>() -> u32 { return 4; }

template<> auto M68K::bits<Byte>() -> u32 { return  8; }
template<> auto M68K::bits<Word>() -> u32 { return 16; }
template<> auto M68K::bits<Long>() -> u32 { return 32; }

template<> auto M68K::lsb<Byte>() -> n32 { return 1; }
template<> auto M68K::lsb<Word>() -> n32 { return 1; }
template<> auto M68K::lsb<Long>() -> n32 { return 1; }

template<> auto M68K::msb<Byte>() -> n32 { return       0x80; }
template<> auto M68K::msb<Word>() -> n32 { return     0x8000; }
template<> auto M68K::msb<Long>() -> n32 { return 0x80000000; }

template<> auto M68K::mask<Byte>() -> n32 { return       0xff; }
template<> auto M68K::mask<Word>() -> n32 { return     0xffff; }
template<> auto M68K::mask<Long>() -> n32 { return 0xffffffff; }

template<> auto M68K::clip<Byte>(n32 data) -> n32 { return (n8 )data; }
template<> auto M68K::clip<Word>(n32 data) -> n32 { return (n16)data; }
template<> auto M68K::clip<Long>(n32 data) -> n32 { return (n32)data; }

template<> auto M68K::sign<Byte>(n32 data) -> i32 { return (i8 )data; }
template<> auto M68K::sign<Word>(n32 data) -> i32 { return (i16)data; }
template<> auto M68K::sign<Long>(n32 data) -> i32 { return (i32)data; }
