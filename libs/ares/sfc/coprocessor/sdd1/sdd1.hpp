struct SDD1 {
  auto unload() -> void;
  auto power() -> void;

  auto ioRead(n24 address, n8 data) -> n8;
  auto ioWrite(n24 address, n8 data) -> void;

  auto dmaRead(n24 address, n8 data) -> n8;
  auto dmaWrite(n24 address, n8 data) -> void;

  auto mmcRead(n24 address) -> n8;

  auto mcuRead(n24 address, n8 data) -> n8;
  auto mcuWrite(n24 address, n8 data) -> void;

  auto serialize(serializer&) -> void;

  ReadableMemory rom;

private:
  n8 r4800;  //hard enable
  n8 r4801;  //soft enable
  n8 r4804;  //MMC bank 0
  n8 r4805;  //MMC bank 1
  n8 r4806;  //MMC bank 2
  n8 r4807;  //MMC bank 3

  struct DMA {
    n24 address;  //$43x2-$43x4 -- DMA transfer address
    n16 size;     //$43x5-$43x6 -- DMA transfer size
  } dma[8];
  n1 dmaReady;    //used to initialize decompression module

public:
  #include "decompressor.hpp"
  Decompressor decompressor;
};

extern SDD1 sdd1;
