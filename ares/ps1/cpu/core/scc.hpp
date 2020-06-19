//{
  //scc-registers.cpp
  auto getControlRegister(u8) -> u32;
  auto setControlRegister(u8, u32) -> void;

  //System Control Coprocessor
  struct SCC {
    // 0: Index

    // 1: Random

    // 2: BusControl
    // 2: EntryLo

    // 3: Config

    // 4: Context

    // 8: BadVirtualAddress

    // 9: Count

    //10: PortSize
    //10: EntryHi

    //11: Compare

    //12: StatusRegister

    //13: Cause

    //14: EPC

    //15: ProductID
  } scc;

  //scc-instructions.cpp
//};
