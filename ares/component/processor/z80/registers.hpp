#define AF af.word
#define BC bc.word
#define DE de.word
#define HL (prefix == Prefix::ix ? ix.word : prefix == Prefix::iy ? iy.word : hl.word)

#define A af.byte.hi
#define F af.byte.lo
#define B bc.byte.hi
#define C bc.byte.lo
#define D de.byte.hi
#define E de.byte.lo
#define H (prefix == Prefix::ix ? ix.byte.hi : prefix == Prefix::iy ? iy.byte.hi : hl.byte.hi)
#define L (prefix == Prefix::ix ? ix.byte.lo : prefix == Prefix::iy ? iy.byte.lo : hl.byte.lo)

#define _HL hl.word  //true HL (ignores IX/IY prefixes)
#define _H  hl.byte.hi
#define _L  hl.byte.lo

#define AF_ af_.word  //shadow registers
#define BC_ bc_.word
#define DE_ de_.word
#define HL_ hl_.word

#define A_ af_.byte.hi
#define F_ af_.byte.lo
#define B_ bc_.byte.hi
#define C_ bc_.byte.lo
#define D_ de_.byte.hi
#define E_ de_.byte.lo
#define H_ hl_.byte.hi
#define L_ hl_.byte.lo

#define IX  ix.word
#define IY  iy.word
#define IR  ir.word
#define WZ  wz.word
#define WZH wz.byte.hi
#define WZL wz.byte.lo

#define I ir.byte.hi
#define R ir.byte.lo

#define CF af.byte.lo.bit(0)
#define NF af.byte.lo.bit(1)
#define PF af.byte.lo.bit(2)
#define VF af.byte.lo.bit(2)
#define XF af.byte.lo.bit(3)
#define HF af.byte.lo.bit(4)
#define YF af.byte.lo.bit(5)
#define ZF af.byte.lo.bit(6)
#define SF af.byte.lo.bit(7)
