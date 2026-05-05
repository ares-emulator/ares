#pragma once

#include <typeinfo>

namespace nall {

struct BCD {
    static auto encode(u8 value) -> u8 { return value / 10 << 4 | value % 10; }
    static auto decode(u8 value) -> u8 { return (value >> 4) * 10 + (value & 15); }
    static auto valid(u8 value) -> bool { return (value & 0x0f) <= 9 && (value >> 4) <= 9; }
};

}
