template<u32 Size> auto AI::read(u32 address, Thread& thread) -> u32 {
    u32 data = 0;
    address = (address & 0x3f) >> 2;

    if (address == 0) {
        // AI_DRAM_ADDRESS
        data = io.dmaAddress[0];
    }

    if (address == 1) {
        // AI_LENGTH
        data = io.dmaLength[0] & 0x3ffff;
    }

    if (address == 3) {
        // AI_STATUS
        data |= (io.dmaCount > 1) << 0;
        data |= (1) << 20;
        data |= (1) << 24;
        data |= (io.dmaEnable) << 25;
        data |= (io.dmaCount > 0) << 30;
        data |= (io.dmaCount > 1) << 31;
    }

    debugger.io(Read, address, data);
    return data;
}

template<u32 Size> auto AI::write(u32 address, u32 data, Thread& thread) -> void {
    address = (address & 0x3f) >> 2;

    if (address == 0) {
        // AI_DRAM_ADDRESS
        if (io.dmaCount < 2) {
            io.dmaAddress[io.dmaCount] = (data & 0xff'ffff) & ~7;
        }
    }

    if (address == 1) {
        // AI_LENGTH
        u32 length = (data & 0x3'ffff) & ~7;
        if (io.dmaCount < 2) {
            if (io.dmaCount == 0) mi.raise(MI::IRQ::AI);
            io.dmaLength[io.dmaCount] = length;
            io.dmaOriginPc[io.dmaCount] = cpu.ipu.pc;
            io.dmaCount++;
        }
    }

    if (address == 2) {
        // AI_CONTROL
        io.dmaEnable = data & 1;
    }

    if (address == 3) {
        // AI_STATUS
        mi.lower(MI::IRQ::AI);
    }

    if (address == 4) {
        // AI_DACRATE
        auto frequency = dac.frequency;
        io.dacRate = data & 0x3fff;
        dac.frequency = nall::max(1, system.videoFrequency() / (io.dacRate + 1));
        dac.period = system.frequency() / dac.frequency;
        if (frequency != dac.frequency) stream->setFrequency(dac.frequency);
    }

    if (address == 5) {
        // AI_BITRATE
        io.bitRate = data & 0xf;
        dac.precision = io.bitRate + 1;
    }

    debugger.io(Write, address, data);
}