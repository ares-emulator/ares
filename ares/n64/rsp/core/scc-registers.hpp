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
      struct Start {
      } start;

      //9: CMD_END
      struct End {
      } end;

      //10: CMD_CURRENT
      struct Current {
      } current;

      //11: CMD_STATUS
      struct Status {
      } status;

      //12: CMD_CLOCK
      struct Clock {
      } clock;

      //13: CMD_BUSY
      struct Busy {
      } busy;

      //14: CMD_PIPE_BUSY
      struct PipeBusy {
      } pipeBusy;

      //15: CMD_TMEM_BUSY
      struct TMEMBusy {
      } tmemBusy;
    } command;
  } scc;
//};
