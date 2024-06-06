auto PPU::incrementVRAMAddressX() -> void {
  if (++var.tileX == 0)
    var.nametableX++;
}

auto PPU::incrementVRAMAddressY() -> void {
  if (++var.fineY == 0 && ++var.tileY == 30)
    var.nametableY++, var.tileY = 0;
}

auto PPU::transferScrollX() -> void {
  var.tileX = scroll.tileX;
  var.nametableX = scroll.nametableX;
}

auto PPU::transferScrollY() -> void {
  var.tileY = scroll.tileY;
  var.nametableY = scroll.nametableY;
  var.fineY = scroll.fineY;
}

auto PPU::cycleScroll() -> void {
  if (io.lx == 0) return;

  if (io.lx <= 256) {
    if (io.lx & 7) return;

    incrementVRAMAddressX();
    if (io.lx == 256) incrementVRAMAddressY();
    return;
  }

  if (io.lx == 257) {
    transferScrollX();
    return;
  }

  if (io.ly == vlines() - 1) {
    if (io.lx >= 280 && io.lx <= 304) {
      transferScrollY();
      return;
    }
  }

  if (io.lx > 320 && io.lx <= 336) {
    if (io.lx & 7) return;

    incrementVRAMAddressX();
    return;
  }
}

auto PPU::scrollTransferDelay() -> void {
  if (!scroll.transferDelay || --scroll.transferDelay)
    return;

  bool isRendering = rendering();
  bool isTransferX = isRendering && io.lx != 0 && (io.lx & 7) == 0 && (io.lx <= 256 || io.lx > 320);
  bool isTransferY = isRendering && io.lx == 256;

  if (isTransferX) {
    scroll.tileX = var.tileX & scroll.tileX;
    scroll.nametableX = var.nametableX & scroll.nametableX;
  }

  if (isTransferY) {
    scroll.tileY = var.tileY & scroll.tileY;
    scroll.nametableY = var.nametableY & scroll.nametableY;
    scroll.fineY = var.fineY & scroll.fineY;
  }

  transferScrollX();
  transferScrollY();

  if (!isRendering) {
    io.busAddress = var.address;
    cartridge.ppuAddressBus(io.busAddress);
  }
}
