#define AF af.word
#define BC bc.word
#define DE de.word
#define HL hl.word
#define WZ  wz.word
#define WZH wz.byte.hi
#define WZL wz.byte.lo

#define A af.byte.hi
#define F af.byte.lo
#define B bc.byte.hi
#define C bc.byte.lo
#define D de.byte.hi
#define E de.byte.lo
#define H  hl.byte.hi
#define L  hl.byte.lo

#define CF af.byte.lo.bit(0)
#define PF af.byte.lo.bit(2)
#define HF af.byte.lo.bit(4)
#define ZF af.byte.lo.bit(6)
#define SF af.byte.lo.bit(7)
