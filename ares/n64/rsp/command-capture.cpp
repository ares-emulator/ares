// F3DEX2 dispatch signature/layout from ares-64 desktop-ui/ui/rsp-overlays/f3dex2.json.
// RSPQ ELF/config and semantic argument decoding remain separate port work.
auto CommandCapture::detectRsp() -> void {
  if(!rspEnabled) return;
  segmentOpen = false; sigPc = loopPc = dispatchPc = -1;
  for(uint32_t pc = 12; pc + 24 <= 4096; pc += 4) {
    if(rsp.imem.read<Word>(pc) == 0x00196603 && rsp.imem.read<Word>(pc + 4) == 0x000c5840) {
      sigPc = pc; loopPc = pc - 12; dispatchPc = pc + 20; break;
    }
  }
}
auto CommandCapture::rspFlush() -> void {
  if(!segmentOpen) return;
  segment.cycle = uint32_t(rsp.pipeline.clocksTotal - uint32_t(segmentStart));
  ++rspObserved;
  if(rspLive.size() < Capacity) rspLive.push_back(segment);
  segmentOpen = false;
}
auto CommandCapture::rspInstruction(uint32_t pc) -> void {
  if(!rspEnabled || sigPc < 0 || (pc != loopPc && pc != dispatchPc)) return;
  // Different (e.g. audio) microcode may execute the same address: revalidate.
  if(rsp.imem.read<Word>(sigPc) != 0x00196603 || rsp.imem.read<Word>(sigPc + 4) != 0x000c5840) { segmentOpen = false; return; }
  rspFlush();
  segment = {}; segment.seq = sequence++; segment.frame = presentedFrame;
  segment.overhead = pc == loopPc;
  if(!segment.overhead) { segment.word0 = rsp.ipu.r[25].u32; segment.word1 = rsp.ipu.r[24].u32; segment.opcode = segment.word0 >> 24; }
  segmentStart = rsp.pipeline.clocksTotal; segmentOpen = true;
}
auto CommandCapture::rspDma(uint32_t bytes, bool toRdram) -> void {
  if(!rspEnabled || !segmentOpen) return;
  if(toRdram) segment.bytesOut += bytes; else segment.bytesIn += bytes;
}
auto CommandCapture::rdpCommand(uint8_t opcode, uint64_t word0, uint64_t word1, uint8_t words) -> void {
  if(!rdpEnabled) return;
  ++rdpObserved;
  auto seq = sequence++;
  if(rdpLive.size() < Capacity) rdpLive.push_back({seq, presentedFrame, word0, word1, opcode, words});
}
auto CommandCapture::afterRun() -> void {
  if(rspEnabled && rsp.status.halted) rspFlush();
  uint32_t origin = vi.io.dramAddress;
  if(!origin || origin == lastOrigin) return;
  lastOrigin = origin;
  rspLatest.swap(rspLive); rspLive.clear(); rspCommitted = rspObserved; rspObserved = 0;
  rdpLatest.swap(rdpLive); rdpLive.clear(); rdpCommitted = rdpObserved; rdpObserved = 0;
  ++presentedFrame; ++generation;
}

auto CommandCapture::rspName(uint8_t opcode) -> const char* {
  switch(opcode) {
  case 0x00: return "NOP";
  case 0x01: return "Vertex";
  case 0x02: return "Modify Vtx";
  case 0x03: return "Cull DL";
  case 0x04: return "Branch Z";
  case 0x05: return "Tri1";
  case 0x06: return "Tri2";
  case 0x07: return "Quad";
  case 0x08: return "Line3D";
  case 0xC8: return "RDP Tri Fill";
  case 0xC9: return "RDP Tri Fill Z";
  case 0xCA: return "RDP Tri Tex";
  case 0xCB: return "RDP Tri Tex Z";
  case 0xCC: return "RDP Tri Shade";
  case 0xCD: return "RDP Tri Shade Z";
  case 0xCE: return "RDP Tri Shade Tex";
  case 0xCF: return "RDP Tri Shade Tex Z";
  case 0xD3: return "Special 3";
  case 0xD4: return "Special 2";
  case 0xD5: return "Special 1";
  case 0xD6: return "DMA IO";
  case 0xD7: return "Texture";
  case 0xD8: return "Pop Matrix";
  case 0xD9: return "Geometry Mode";
  case 0xDA: return "Matrix";
  case 0xDB: return "Move Word";
  case 0xDC: return "Move Mem";
  case 0xDD: return "Load Ucode";
  case 0xDE: return "Call DL";
  case 0xDF: return "End DL";
  case 0xE0: return "SP NOP";
  case 0xE1: return "RDP Half 1";
  case 0xE2: return "Set Mode L";
  case 0xE3: return "Set Mode H";
  case 0xE4: return "Tex Rect";
  case 0xE5: return "Tex Rect Flip";
  case 0xE6: return "Load Sync";
  case 0xE7: return "Pipe Sync";
  case 0xE8: return "Tile Sync";
  case 0xE9: return "Full Sync";
  case 0xEA: return "Set Key GB";
  case 0xEB: return "Set Key R";
  case 0xEC: return "Set Convert";
  case 0xED: return "Set Scissor";
  case 0xEE: return "Set Prim Depth";
  case 0xEF: return "Set Other Modes";
  case 0xF0: return "Load TLUT";
  case 0xF1: return "RDP Half 2";
  case 0xF2: return "Set Tile Size";
  case 0xF3: return "Load Block";
  case 0xF4: return "Load Tile";
  case 0xF5: return "Set Tile";
  case 0xF6: return "Fill Rect";
  case 0xF7: return "Set Fill Color";
  case 0xF8: return "Set Fog Color";
  case 0xF9: return "Set Blend Color";
  case 0xFA: return "Set Prim Color";
  case 0xFB: return "Set Env Color";
  case 0xFC: return "Set Combine";
  case 0xFD: return "Set Tex Image";
  case 0xFE: return "Set Z Image";
  case 0xFF: return "Set Color Image";
  default: return "Unknown";
  }
}

auto CommandCapture::rdpName(uint8_t opcode) -> const char* {
  static const char* names[] = {
  "No_Operation", "Invalid_01", "Invalid_02", "Invalid_03",
  "Invalid_04",   "Invalid_05", "Invalid_06", "Invalid_07",
  "Unshaded_Triangle",
  "Unshaded_Zbuffer_Triangle",
  "Texture_Triangle",
  "Texture_Zbuffer_Triangle",
  "Shaded_Triangle",
  "Shaded_Zbuffer_Triangle",
  "Shaded_Texture_Triangle",
  "Shaded_Texture_Zbuffer_Triangle",
  "Invalid_10", "Invalid_11", "Invalid_12", "Invalid_13",
  "Invalid_14", "Invalid_15", "Invalid_16", "Invalid_17",
  "Invalid_18", "Invalid_19", "Invalid_1a", "Invalid_1b",
  "Invalid_1c", "Invalid_1d", "Invalid_1e", "Invalid_1f",
  "Invalid_20", "Invalid_21", "Invalid_22", "Invalid_23",
  "Texture_Rectangle",
  "Texture_Rectangle_Flip",
  "Sync_Load",
  "Sync_Pipe",
  "Sync_Tile",
  "Sync_Full",
  "Set_Key_GB",
  "Set_Key_R",
  "Set_Convert",
  "Set_Scissor",
  "Set_Primitive_Depth",
  "Set_Other_Modes",
  "Load_Texture_LUT",
  "Invalid_31",
  "Set_Tile_Size",
  "Load_Block",
  "Load_Tile",
  "Set_Tile",
  "Fill_Rectangle",
  "Set_Fill_Color",
  "Set_Fog_Color",
  "Set_Blend_Color",
  "Set_Primitive_Color",
  "Set_Environment_Color",
  "Set_Combine_Mode",
  "Set_Texture_Image",
  "Set_Mask_Image",
  "Set_Color_Image",
};
  return names[opcode & 63];
}
