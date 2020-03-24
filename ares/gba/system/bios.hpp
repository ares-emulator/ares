struct BIOS {
  BIOS();
  ~BIOS();

  auto read(uint mode, uint32 addr) -> uint32;
  auto write(uint mode, uint32 addr, uint32 word) -> void;

  uint8* data = nullptr;
  uint size = 0;
  uint32 mdr = 0;
};

extern BIOS bios;
