auto AI::serialization(serializer& s) -> void {
	s(io.dacRate);
	s(io.bitRate);
	s(io.dmaEnable);
	s(io.dmaCount);
	for (auto& address : io.dmaAddress) s(address);
	for (auto& length : io.dmaLength) s(length);
	for (auto& pc : io.dmaOriginPc) s(pc);
	s(dac.frequency);
	s(dac.precision);
	s(dac.period);
	s(outputLeft);
	s(outputRight);
}