struct Mega32X : Interface {
  using Interface::Interface;
  std::unique_ptr<Board::Interface> board;

  auto load() -> void override {
    if(pak) {
      board = std::make_unique<Board::Standard>(*cartridge);
    } else {
      board = std::make_unique<Board::Interface>(*cartridge);
    }

    board->pak = pak;
    board->load();

    cartridge->child = *board;
  }

  auto save() -> void override {
    board->save();
  }

  auto read(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(!m32x.io.adapterEnable) {
      return board->read(upper, lower, address, data);
    }

    if(address >= 0x000000 && address <= 0x0000ff) {
      return m32x.readExternal(upper, lower, address, data);
    }

    if(m32x.dreq.vram) {
      return board->read(upper, lower, address, data);
    }

    return data;
  }

  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(!m32x.io.adapterEnable) {
      return board->write(upper, lower, address, data);
    }

    if(address >= 0x000000 && address <= 0x0000ff) {
      return m32x.writeExternal(upper, lower, address, data);
    }

    if(m32x.dreq.vram) {
      return board->write(upper, lower, address, data);
    }
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    data = board->readIO(upper, lower, address, data);

    return m32x.readExternalIO(upper, lower, address, data);
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    board->writeIO(upper, lower, address, data);

    return m32x.writeExternalIO(upper, lower, address, data);
  }

  auto vblank(bool line) -> void override {
    return m32x.vblank(line);
  }

  auto hblank(bool line) -> void override {
    return m32x.hblank(line);
  }

  auto power(bool reset) -> void override {
    board->power(reset);
  }

  auto serialize(serializer& s) -> void override {
    s(*board);
  }
};
