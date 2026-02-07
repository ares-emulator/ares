auto AI::serialization(serializer& s) -> void {
	s.integer(io.dacRate);
	s.integer(io.bitRate);
	s.integer(io.dmaEnable);
	s.integer(io.dmaCount);
	for (auto& address : io.dmaAddress) s.integer(address);
	for (auto& length : io.dmaLength) s.integer(length);
	for (auto& pc : io.dmaOriginPc) s.integer(pc);
	s.integer(dac.frequency);
	s.integer(dac.precision);
	s.integer(dac.period);	
	s.real(outputLeft);
	s.real(outputRight);
}