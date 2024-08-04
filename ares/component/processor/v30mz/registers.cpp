auto V30MZ::segment(u16 segment) -> u16 {
  if(prefix.segment == SegmentOverrideDS1) return DS1;
  if(prefix.segment == SegmentOverridePS ) return PS;
  if(prefix.segment == SegmentOverrideSS ) return SS;
  if(prefix.segment == SegmentOverrideDS0) return DS0;
  return segment;
}

template<> auto V30MZ::getAccumulator<Byte>() -> u32 { return AL; }
template<> auto V30MZ::getAccumulator<Word>() -> u32 { return AW; }
template<> auto V30MZ::getAccumulator<Long>() -> u32 { return AW | DW << 16; }

template<> auto V30MZ::setAccumulator<Byte>(u32 data) -> void { AL = data; }
template<> auto V30MZ::setAccumulator<Word>(u32 data) -> void { AW = data; }
template<> auto V30MZ::setAccumulator<Long>(u32 data) -> void { AW = data; DW = data >> 16; }
