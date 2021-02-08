struct BIOS {
  BIOS();
  ~BIOS();

  auto read(u32 mode, n32 address) -> n32;
  auto write(u32 mode, n32 address, n32 word) -> void;

  n8* data = nullptr;
  u32 size = 0;
  n32 mdr = 0;
};

extern BIOS bios;
