#pragma once
#include "capture-elf.hpp"
#include "capture-dwarf.hpp"
#include "capture-rdp-decode.hpp"
#include <nall/string/markup/json.hpp>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <map>
#include <charconv>
#include <limits>
// Selective RSPCapture config/ELF/argument port from ares-64 1c9b9637.
// No filesystem access from live hooks; all descriptors are bounded and parsed once.
struct CaptureConfig {
  using Node = nall::Markup::Node;
  using Capture = ares::Nintendo64::CommandCapture;
  struct Field {
    std::string name; unsigned word = 0, hi = 23, lo = 0; bool sign = false;
    int64_t mul = 1, div = 1, add = 0; double scale = 0;
    std::map<int64_t, std::string> enums;
  };
  struct Command { std::string name, fmt; std::vector<Field> fields; };
  struct Overlay { std::string name; int id = -1; std::map<unsigned, Command> commands; };
  struct Ucode { uint32_t start, end; std::string name; };
  Capture::QueueConfig queue;
  Capture::DisplayConfig display;
  std::string displayName = "F3DEX2", diagnostic = "no companion RSPQ ELF";
  std::map<unsigned, Command> displayCommands;
  std::vector<Overlay> overlays;
  std::vector<Ucode> ucodes;
  std::vector<nall::Decode::DWARF::Field> labels;
  uint32_t table = 8, idmap = 0x48;
  bool tableLocked = false;
  static constexpr const char* Builtin =
#include "capture-data/rspq-libdragon.inc"
  ;
  static std::string text(Node n) { return n.value().data(); }
  static std::string bounded(Node n, size_t max = 64) {
    auto value = text(n); if(value.size() > max) throw std::runtime_error("descriptor string too long");
    for(unsigned char c : value) if(c < 32 || c > 126) throw std::runtime_error("descriptor string must be printable ASCII");
    return value;
  }
  static int64_t number(const std::string& s) {
    if(s.empty()) throw std::runtime_error("missing descriptor number");
    bool neg = s[0] == '-'; size_t p = neg ? 1 : 0; int base = 10;
    if(s.substr(p,2) == "0x" || s.substr(p,2) == "0X") { p += 2; base = 16; }
    uint64_t value; auto r = std::from_chars(s.data()+p,s.data()+s.size(),value,base);
    if(r.ec != std::errc{} || r.ptr != s.data()+s.size() || value > INT64_MAX) throw std::runtime_error("invalid descriptor number");
    return neg ? -int64_t(value) : int64_t(value);
  }
  static int64_t integer(Node n, int64_t fallback) { return n ? number(text(n)) : fallback; }
  static uint32_t address(Node n, uint32_t max = 4092) {
    auto v = number(text(n)); if(v < 0 || v > max || v & 3) throw std::runtime_error("invalid descriptor address"); return v;
  }
  static std::string read(const std::filesystem::path& path, size_t max) {
    std::error_code ec; if(!std::filesystem::is_regular_file(path,ec)) return {};
    auto size = std::filesystem::file_size(path,ec); if(ec || !size || size > max) return {};
    std::ifstream in(path,std::ios::binary); std::string result(size,'\0');
    if(!in.read(result.data(),size)) return {}; return result;
  }
  static Node json(const std::string& data) {
    if(data.empty() || data.size() > 131072) throw std::runtime_error("descriptor size");
    // Bound parser recursion and reject embedded NUL before the nall parser.
    int depth = 0; bool quoted = false, escape = false;
    for(size_t index=0;index<data.size();++index) {
      char c=data[index];
      if(!c) throw std::runtime_error("descriptor NUL");
      if(quoted) { if(escape) escape = false; else if(c == '\\') escape = true; else if(c == '"') quoted = false; }
      else if(c == '"') quoted = true;
      else if(c == 'n' || c == 't' || c == 'f') {
        const char* literal=c=='n' ? "null" : c=='t' ? "true" : "false";
        if(data.compare(index,std::strlen(literal),literal)) throw std::runtime_error("invalid JSON literal");
      }
      else if(c == '[' || c == '{') { if(++depth > 16) throw std::runtime_error("descriptor depth"); }
      else if(c == ']' || c == '}') { if(--depth < 0) throw std::runtime_error("descriptor brackets"); }
    }
    auto result = nall::JSON::unserialize(data.c_str()); if(!result || depth || quoted) throw std::runtime_error("invalid descriptor JSON"); return result;
  }
  static Command command(Node n) {
    Command c; c.name = bounded(n["name"]); c.fmt = bounded(n["fmt"],256);
    if(n["args"].size() > 32) throw std::runtime_error("too many argument fields");
    for(auto a : n["args"]) {
      Field f; f.name = bounded(a["name"],32); f.word = integer(a["word"],0);
      if(f.name.empty() || f.word >= 16) throw std::runtime_error("invalid argument field");
      f.hi = f.word ? 31 : 23;
      if(a["bits"]) { if(a["bits"].size() != 2) throw std::runtime_error("invalid field bits"); f.hi = number(text(a["bits"]["0"])); f.lo = number(text(a["bits"]["1"])); }
      if(f.hi > 31 || f.lo > f.hi) throw std::runtime_error("invalid field bits");
      f.sign = a["signed"].boolean(false); f.mul = integer(a["mul"],1); f.div = integer(a["div"],1); f.add = integer(a["add"],0);
      if(!f.div || std::abs((long double)f.mul) > 1000000 || std::abs((long double)f.add) > 1000000000 || std::abs((long double)f.div) > 1000000) throw std::runtime_error("invalid field transform");
      auto real = [](Node v) { auto s = text(v); char* end; double d = std::strtod(s.c_str(),&end); if(s.empty() || *end || !std::isfinite(d)) throw std::runtime_error("invalid float transform"); return d; };
      if(a["fdiv"]) { double d = real(a["fdiv"]); if(!d) throw std::runtime_error("zero float divisor"); f.scale = 1 / d; }
      if(a["fmul"]) f.scale = (f.scale ? f.scale : 1) * real(a["fmul"]);
      if(a["fscale"]) f.scale = real(a["fscale"]);
      if(!std::isfinite(f.scale) || std::abs(f.scale) > 1e12) throw std::runtime_error("float transform range");
      if(a["enum"].size() > 64) throw std::runtime_error("too many enum entries");
      for(auto e : a["enum"]) f.enums.emplace(number(e.name().data()),bounded(e));
      c.fields.push_back(std::move(f));
    }
    return c;
  }
  static std::map<unsigned, Command> commands(Node n) {
    if(n.size() > 256) throw std::runtime_error("too many commands");
    std::map<unsigned, Command> out;
    for(auto entry : n) { auto id = number(entry.name().data()); if(id < 0 || id > 255 || !out.emplace(id,command(entry)).second) throw std::runtime_error("invalid command id"); }
    return out;
  }
  bool loadQueue(const std::string& data) {
    try {
      auto j = json(data); auto next = queue;
      const char* keys[] = {"rspq_execute_command","RSPQ_Loop","rspq_load_overlay","rspq_fetch_buffer","rspq_fetch_buffer_with_ptr"};
      for(unsigned i = 0; i < 5; ++i) next.hooks[i] = address(j["imem"][keys[i]],4088);
      next.cmdsOffset = address(j["dmem"]["cmds_offset"]); next.cmdsSize = address(j["dmem"]["cmds_size"],4096);
      if(!next.cmdsSize || next.cmdsSize > 4096-next.cmdsOffset) throw std::runtime_error("invalid command buffer");
      auto nextTable = address(j["dmem"]["ovl_table_offset"],4000);
      auto nextIdmap = integer(j["dmem"]["ovl_idmap_offset"],nextTable+64);
      if(nextIdmap < 0 || nextIdmap > 4080) throw std::runtime_error("invalid overlay idmap");
      std::vector<Overlay> list; if(j["overlays"].size() > 16) throw std::runtime_error("too many overlays");
      for(auto o : j["overlays"]) { Overlay overlay; overlay.name = bounded(o["name"]); overlay.id = integer(o["id"],-1); if(overlay.id < -1 || overlay.id > 15) throw std::runtime_error("invalid overlay id"); overlay.commands = commands(o["commands"]); list.push_back(std::move(overlay)); }
      queue = next; overlays = std::move(list); table = nextTable; idmap = nextIdmap; return true;
    } catch(const std::exception& e) { diagnostic = e.what(); return false; }
  }
  bool loadDisplay(const std::string& data) {
    try {
      auto j = json(data); if(text(j["mode"]) != "displaylist") return false;
      Capture::DisplayConfig next; auto d = j["dispatch"];
      if(d["signature"].size() < 1 || d["signature"].size() > 4) throw std::runtime_error("invalid display signature");
      next.count = d["signature"].size(); unsigned i = 0;
      for(auto w : d["signature"]) { auto v = number(text(w)); if(v < 0 || v > UINT32_MAX) throw std::runtime_error("invalid signature word"); next.signature[i++] = v; }
      auto loop = integer(d["loopOffset"],0), dispatch = integer(d["dispatchOffset"],0);
      auto r0 = integer(d["word0Reg"],25), r1 = integer(d["word1Reg"],24), shift = integer(d["opcodeShift"],24);
      if(loop < -4092 || loop > 4092 || dispatch < -4092 || dispatch > 4092 || loop % 4 || dispatch % 4 || loop == dispatch || r0 < 0 || r0 > 31 || r1 < 0 || r1 > 31 || shift < 0 || shift > 24) throw std::runtime_error("invalid display dispatch");
      next.loopOffset = loop; next.dispatchOffset = dispatch; next.word0Reg = r0; next.word1Reg = r1; next.opcodeShift = shift;
      auto list = commands(j["commands"]); auto name = bounded(j["banner"] ? j["banner"] : j["name"]);
      if(name.empty()) throw std::runtime_error("missing display name");
      display = next; displayName = name; displayCommands = std::move(list); return true;
    } catch(const std::exception& e) { diagnostic = e.what(); return false; }
  }
  void discover(const std::string& rom) {
    *this = {}; namespace fs = std::filesystem; fs::path base(rom); base.replace_extension();
    auto dir = base.parent_path(); if(dir.empty()) dir = ".";
    fs::path elfPath;
    for(auto candidate : {fs::path(base.string()+".elf"), base/"build"/(base.filename().string()+".elf"), dir/"build"/(base.filename().string()+".elf")}) {
      auto bytes = read(candidate,64*1024*1024); if(bytes.empty()) continue;
      nall::Decode::ELF elf((const u8*)bytes.data(),bytes.size());
      if(!elf || !elf.bigEndian()) { diagnostic = "invalid N64 companion ELF"; continue; }
      u64 queueText = 0;
      if(!elf.symbolValue("rsp_queue_text_start",queueText)) { diagnostic = "ELF has no rsp_queue_text_start"; continue; }
      auto custom = read(candidate.parent_path()/"rspq-libdragon.json",131072);
      if(!loadQueue(custom.empty() ? Builtin : custom)) continue;
      bool valid = true;
      for(unsigned h = 0; h < 5; ++h) for(unsigned w = 0; w < 2; ++w) {
        u32 value; if(!elf.readWord(queueText + queue.hooks[h] + w*4,value)) valid = false; else queue.signature[h][w] = value;
      }
      if(!valid) { diagnostic = "ELF queue hook is not file-backed"; continue; }
      nall::Decode::DWARF dwarf(elf);
      labels = dwarf.flattenStruct("rsp_queue_s");
      uint32_t dataOffset=8,endOffset=12,nameOffset=24;
      for(auto& field : dwarf.flattenStruct("rsp_ucode_t")) {
        if(field.offset > 4092) continue;
        if(field.name == "data") dataOffset=field.offset;
        else if(field.name == "data_end") endOffset=field.offset;
        else if(field.name == "name") nameOffset=field.offset;
      }
      for(auto& sym : elf.symbols()) {
        if(ucodes.size() >= 256) break;
        if(!sym.name.beginsWith("rsp_") || sym.value < 0x80000000 || sym.value > 0x80ffffff) continue;
        u32 start,end,name; if(!elf.readWord(sym.value+dataOffset,start) || !elf.readWord(sym.value+endOffset,end) || !elf.readWord(sym.value+nameOffset,name)) continue;
        if(start < 0x80000000 || end > 0x81000000 || start >= end) continue;
        u64 avail = 0; auto ptr = elf.virtualAddress(name,avail); if(!ptr) continue;
        auto nul = (const uint8_t*)std::memchr(ptr,0,std::min<uint64_t>(avail,64)); if(!nul) continue;
        std::string label((const char*)ptr,nul-ptr); if(label.starts_with("rsp_")) label.erase(0,4);
        if(label.empty() || std::any_of(label.begin(),label.end(),[](unsigned char c){return c < 32 || c > 126;})) continue;
        ucodes.push_back({start & 0xffffff,end & 0xffffff,label});
      }
      queue.loaded = true; diagnostic = "ELF-validated RSPQ hooks"; elfPath = candidate; break;
    }
    auto customDisplay = read((elfPath.empty() ? dir : elfPath.parent_path())/"f3dex2.json",131072);
    if(!customDisplay.empty()) loadDisplay(customDisplay);
  }
  std::string match(uint32_t physical) const {
    if(!physical) return {};
    for(auto& u : ucodes) if(physical >= u.start && physical < u.end) return u.name;
    const Ucode* best = nullptr;
    for(auto& u : ucodes) if(physical >= u.start && physical-u.start < 4096 && (!best || best->start < u.start)) best = &u;
    return best ? best->name : "";
  }
  std::pair<std::string,unsigned> resolve(unsigned id) {
    using namespace ares::Nintendo64;
    if(!id) { for(auto& o : overlays) if(o.id == 0) return {o.name,0}; return {"RSPQ",0}; }
    if(!tableLocked && !ucodes.empty()) {
      auto score = [&](uint32_t off) { unsigned n = 0; for(unsigned i=1;i<16;++i) { auto e = rsp.dmem.read<Byte>(off+64+i); if(e && !(e&3) && e < 64 && !match(rsp.dmem.read<Word>(off+e)&0xffffff).empty()) ++n; } return n; };
      auto best = table, bestScore = score(table);
      for(unsigned off = 0; off <= 64; off += 4) if(auto n=score(off); n>bestScore) { best=off;bestScore=n; }
      if(bestScore >= 2) { table=best;idmap=best+64;tableLocked=true; }
    }
    auto slot = rsp.dmem.read<Byte>(idmap+id);
    std::string name;
    if(slot && !(slot&3) && slot < 64) name = match(rsp.dmem.read<Word>(table+slot)&0xffffff);
    if(name.empty()) { for(auto& o : overlays) if(o.id == int(id)) return {o.name,0}; return {"Overlay " + std::to_string(id),0}; }
    unsigned base = id;
    while(base>1 && rsp.dmem.read<Byte>(idmap+base-1) == slot) --base;
    return {name,(id-base)*16};
  }
  static int64_t extract(const Field& f, const Capture::RspRecord& r) {
    unsigned width = f.hi-f.lo+1; uint64_t mask = width == 32 ? UINT32_MAX : (uint64_t(1)<<width)-1;
    uint64_t bits = ((f.word < r.wordCount ? r.words[f.word] : 0) >> f.lo) & mask;
    int64_t value = f.sign && (bits & (uint64_t(1)<<(width-1))) ? int64_t(bits)-int64_t(uint64_t(1)<<width) : bits;
    return value*f.mul/f.div+f.add;
  }
  std::string format(const Command& c, const Capture::RspRecord& r) const {
    auto value = [&](const Field& f, const std::string& spec) {
      if(f.word >= r.wordCount) return std::string("<missing>");
      auto v = extract(f,r); char out[64];
      if(spec == "dmem") for(auto& label : labels) {
        if(v < 0 || uint64_t(v) < label.offset || uint64_t(v)-label.offset >= label.size) continue;
        auto rel=uint64_t(v)-label.offset;std::string name=label.name.data();
        if(label.stride) {name += "["+std::to_string(rel/label.stride)+"]";rel%=label.stride;}
        if(rel) name += "+"+std::to_string(rel);if(name.size()>128)name.resize(128);return name;
      }
      if(spec == "x" || spec == "dmem") { std::snprintf(out,sizeof out,"0x%X",uint32_t(v)); return std::string(out); }
      if(spec == "f" || (spec.empty() && f.scale)) { std::snprintf(out,sizeof out,"%.4g",double(v)*(f.scale ? f.scale : 1)); return std::string(out); }
      if(spec != "d") if(auto it=f.enums.find(v);it!=f.enums.end()) return it->second;
      return std::to_string(v);
    };
    std::string result;
    if(c.fmt.empty()) for(auto& f : c.fields) { if(!result.empty()) result += ' '; result += f.name+'='+value(f,""); }
    else for(size_t i=0;i<c.fmt.size();) {
      if(c.fmt[i]!='{') { result+=c.fmt[i++];continue; }
      auto end=c.fmt.find('}',i);if(end==std::string::npos) { result+=c.fmt.substr(i);break; }
      auto token=c.fmt.substr(i+1,end-i-1), name=token, spec=std::string{};auto colon=token.find(':');
      if(colon!=std::string::npos) { name=token.substr(0,colon);spec=token.substr(colon+1); }
      auto it=std::find_if(c.fields.begin(),c.fields.end(),[&](auto& f){return f.name==name;});
      if(name == "rdp" && r.wordCount >= 2) {
        unsigned opcode = 0; auto decoded = std::from_chars(spec.data(),spec.data()+spec.size(),opcode,16);
        if(decoded.ec == std::errc{} && decoded.ptr == spec.data()+spec.size() && opcode < 64) {
          std::string description = CaptureRdpDecode::rdpCommandDescription(opcode,(uint64_t(r.words[0]&0xffffff)<<32)|r.words[1]).data();
          std::replace(description.begin(),description.end(),'\n',' '); result += description;
        } else result += "{"+token+"}";
      } else result += it!=c.fields.end() ? value(*it,spec) : "{"+token+"}"; i=end+1;
    }
    if(result.size()>256) result=result.substr(0,253)+"...";
    return result;
  }
  void annotate(Capture::RspRecord& r) {
    auto& capture=ares::Nintendo64::commandCapture;
    if(r.overhead) { r.overlay=capture.queueMode ? "RSPQ" : displayName; r.command=r.segmentType==3 ? "Overlay load" : r.segmentType==4 ? "Buffer fetch" : "Dispatch loop"; return; }
    const Command* cmd=nullptr;
    if(capture.queueMode) {
      auto [name,offset]=resolve(r.overlayId);r.overlay=name;
      for(auto& o : overlays) if(o.name==name) if(auto it=o.commands.find(r.opcode+offset);it!=o.commands.end()) { cmd=&it->second;break; }
      r.command=cmd && !cmd->name.empty() ? cmd->name : "Command "+std::to_string(r.opcode+offset);
    } else { r.overlay=displayName; if(auto it=displayCommands.find(r.opcode);it!=displayCommands.end()) cmd=&it->second; r.command=cmd && !cmd->name.empty() ? cmd->name : Capture::rspName(r.opcode); }
    if(cmd) r.args=format(*cmd,r);
  }
  void apply() {
    auto& capture=ares::Nintendo64::commandCapture; capture.queue=queue;capture.display=display;
    capture.annotate=[this](Capture::RspRecord& r){annotate(r);};
  }
};
