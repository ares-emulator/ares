//{
  //scc-registers.cpp
  auto getControlRegister(uint5) -> u64;
  auto setControlRegister(uint5, uint64) -> void;

  //System Control Coprocessor
  struct SCC {
    struct DMA {
      //0: DMA_CACHE
      uint12 memAddress = 0;
       uint1 memSource = 0;

      //1: DMA_DRAM
      uint24 dramAddress = 0;

      //2: DMA_READ_LENGTH
      struct Read {
        uint12 length = 0;
         uint8 count = 0;
        uint12 skip = 0;
      } read;

      //3: DMA_WRITE_LENGTH
      struct Write {
        uint12 length = 0;
         uint8 count = 0;
        uint12 skip = 0;
      } write;

      //4: SP_STATUS
      uint1 busy = 0;
      uint1 full = 0;
    } dma;

    //4: SP_STATUS
    struct Status {
      uint1 halted = 1;
      uint1 broken = 0;
      uint1 ioFull = 0;
      uint1 singleStep = 0;
      uint1 interruptOnBreak = 0;
      uint1 signal[8] = {};
    } status;

    //5: DMA_FULL
    //maps to SP_STATUS.DMA_FULL

    //6: DMA_BUSY
    //maps to SP_STATUS.DMA_BUSY

    //7: SP_SEMAPHORE
    uint1 semaphore = 0;

    struct Command {
      //8: CMD_START
      uint24 start = 0;

      //9: CMD_END
      uint24 end = 0;

      //10: CMD_CURRENT
      uint24 current = 0;

      //11: CMD_STATUS
      struct Status {
        uint1 source = 0;  //0 = RDRAM, 1 = DMEM
      } status;

      //12: CMD_CLOCK
      uint24 clock = 0;

      //13: CMD_BUSY
      uint24 bufferBusy = 0;

      //14: CMD_PIPE_BUSY
      uint24 pipeBusy = 0;

      //15: CMD_TMEM_BUSY
      uint24 tmemBusy = 0;
    } command;
  } scc;

  //scc-instructions.cpp
  auto instructionMFC0() -> void;
  auto instructionMTC0() -> void;
//};
