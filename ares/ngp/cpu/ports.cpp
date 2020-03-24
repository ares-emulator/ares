/* Port 1: (input/output)
 * This port is used to control d8-d15 for 8-bit bus accesses.
 */

CPU::P1::operator bool() const {
  return latch;
}

auto CPU::P1::operator=(bool value) -> void {
  latch = value;
}

/* Port 2: (output)
 * This port is used to control a16-a23 of the address bus.
 */

CPU::P2::operator bool() const {
  return latch;
}

auto CPU::P2::operator=(bool value) -> void {
  latch = value;
}

/* Port 5: (input/output)
 * This port is used for bus monitoring.
 */

CPU::P5::operator bool() const {
  return latch;
}

auto CPU::P5::operator=(bool value) -> void {
  latch = value;
}

/* Port 6: (output)
 * This port provides chip select information.
 */

CPU::P6::operator bool() const {
  return latch;
}

auto CPU::P6::operator=(bool value) -> void {
  latch = value;
}

/* Port 7: (input/output)
 * This port is for the pattern generator.
 */

CPU::P7::operator bool() const {
  return latch;
}

auto CPU::P7::operator=(bool value) -> void {
  latch = value;
}

/* Port 8: (input/output)
 * This port is used for serial communications.
 */

CPU::P8F::operator bool() const {
  return latch;
}

auto CPU::P8F::operator=(bool value) -> void {
  latch = value;
}

CPU::P8M::operator bool() const {
  return latch;
}

auto CPU::P8M::operator=(bool value) -> void {
  latch = value;
}

/* Port 9: (input)
 * This port is used for analog to digital conversion measurements.
 */

CPU::P9::operator bool() const {
  return latch;
}

/* Port A: (input/output)
 * This port controls 8-bit timers and external interrupts.
 */

// PA0
// /WAIT

CPU::PA0::operator bool() const {
  return latch;
}

auto CPU::PA0::operator=(bool value) -> void {
  latch = value;
}

// PA1
// TI0
// (/HBLANK)

CPU::PA1::operator bool() const {
  if(!readable()) return cpu.ti0;
  return latch;
}

auto CPU::PA1::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.ti0 = latch;
}

// PA2
// TO1

CPU::PA2::operator bool() const {
  return latch;
}

auto CPU::PA2::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.to1 = latch;
}

// PA3
// TO3

CPU::PA3::operator bool() const {
  return latch;
}

auto CPU::PA3::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.to3 = latch;
}

/* Port B: (input/output)
 * This port controls 16-bit timers and external interrupts.
 */

// PB0
// INT4
// TI4
// (/VBLANK)

CPU::PB0::operator bool() const {
  return latch;
}

auto CPU::PB0::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.int4 = latch;
  cpu.ti4 = latch;
}

// PB1
// INT5
// TI5
// (APU)

CPU::PB1::operator bool() const {
  return latch;
}

auto CPU::PB1::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.int5 = latch;
  cpu.ti5 = latch;
}

// PB2
// TO4

CPU::PB2::operator bool() const {
  return latch;
}

auto CPU::PB2::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.to4 = latch;
}

// PB3
// TO5

CPU::PB3::operator bool() const {
  return latch;
}

auto CPU::PB3::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.to5 = latch;
}

// PB4
// INT6
// TI6

CPU::PB4::operator bool() const {
  return latch;
}

auto CPU::PB4::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.int6 = latch;
  cpu.ti6 = latch;
}

// PB5
// INT7
// TI7

CPU::PB5::operator bool() const {
  return latch;
}

auto CPU::PB5::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.int7 = latch;
  cpu.ti7 = latch;
}

// PB6
// TO6

CPU::PB6::operator bool() const {
  return latch;
}

auto CPU::PB6::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.to6 = latch;
}

// PB7
// INT0

CPU::PB7::operator bool() const {
  return latch;
}

auto CPU::PB7::operator=(bool value) -> void {
  latch = value;
  if(!writable()) return;
  cpu.int0 = latch;
}
