#pragma once
#include <map>
#include <deque>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "memory-service.hpp"

// Native immutable bytes and bounded frame rings. All entry points run only on
// the emulation thread. This reports content changes, never writes that cancel out.
struct MemoryTools {
  struct Range { MemoryService::Region* region; uint32_t offset, length; };
  struct Sample { uint64_t sequence, frame; std::vector<std::vector<uint8_t>> bytes; };
  struct Capture {
    std::string owner; uint64_t epoch, id, next = 0, dropped = 0, duration = 0;
    uint64_t lastCopyNs = 0, maxCopyNs = 0, copiedBytes = 0, transferredBytes = 0;
    uint32_t capacity = 1; bool recording = false, active = false, pending = false;
    std::vector<Range> ranges; std::deque<Sample> samples;
  };
  MemoryService* memory = nullptr;
  uint64_t nextId = 0, epoch = 0, observedFrame = 0;
  std::string observedKind = "unavailable";
  std::map<uint64_t, Capture> captures;
  static constexpr size_t budget = 64 * 1024 * 1024;
  size_t reserved() const { size_t n = 0; for(auto& [id,c] : captures) for(auto& r:c.ranges) n += size_t(r.length) * c.capacity; return n; }
  void sync(uint64_t current) { if(epoch != current) { captures.clear(); queries.clear(); epoch = current; observedFrame = 0; } }
  void releaseOwner(const std::string& owner) { for(auto i = queries.begin(); i != queries.end();) if(i->second.owner == owner) i = queries.erase(i); else ++i; for(auto i = captures.begin(); i != captures.end();) if(i->second.owner == owner) i = captures.erase(i); else ++i; }
  static std::string hex(const std::vector<uint8_t>& b, size_t offset, size_t length) {
    const char* digits="0123456789abcdef"; std::string out; out.reserve(length*2);
    for(size_t i=offset;i<offset+length;++i) { out += digits[b[i]>>4]; out += digits[b[i]&15]; } return out;
  }
  std::string validate(const std::vector<std::string>& a, size_t begin, std::vector<Range>& ranges, size_t maxBytes) {
    if(a.size() <= begin || (a.size()-begin)%3 || (a.size()-begin)/3 > 64) return "invalid_request";
    size_t total=0;
    for(size_t i=begin;i<a.size();i+=3) {
      uint32_t offset,length;
      if(!MemoryService::number(a[i+1],offset)||!MemoryService::number(a[i+2],length)||!length||length>maxBytes-total) return "invalid_request";
      MemoryService::Region* region=nullptr; for(auto& r:memory->regions) if(r.id==a[i]) {region=&r;break;}
      if(!region) return "stale_region";
      if(!region->supported) return "unsupported_region";
      if(!region->node->readable()) return "region_not_readable";
      if(offset>region->node->size()||length>region->node->size()-offset) return "out_of_bounds";
      ranges.push_back({region,offset,length}); total+=length;
    }
    return {};
  }
  void sample(Capture& c,uint64_t frame) {
    const auto started = std::chrono::steady_clock::now();
    Sample s{c.next++,frame,{}};
    for(auto& r:c.ranges) { std::vector<uint8_t> bytes(r.length); for(uint32_t i=0;i<r.length;++i) bytes[i]=r.region->read(r.offset+i); s.bytes.push_back(std::move(bytes)); }
    c.lastCopyNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count();
    c.maxCopyNs = std::max(c.maxCopyNs, c.lastCopyNs);
    for(const auto& b : s.bytes) c.copiedBytes += b.size();
    if(c.samples.size()==c.capacity) {c.samples.pop_front();++c.dropped;}
    c.samples.push_back(std::move(s));
    if(!c.recording || (c.duration && c.next>=c.duration)) c.active=false;
    c.pending=false;
  }
  void boundary(uint64_t current,uint64_t frame) { sync(current); for(auto& [id,c]:captures) if(c.active) sample(c,frame); }
  std::string metadata(const Capture& c,const std::string& kind) {
    std::string out="{\"handle\":"+std::to_string(c.id)+",\"epoch\":"+std::to_string(c.epoch)+",\"frame_kind\":"+controlQuote(kind)+",\"active\":"+(c.active?"true":"false")+",\"pending\":"+(c.pending?"true":"false")+",\"capacity\":"+std::to_string(c.capacity)+",\"duration\":"+std::to_string(c.duration)+",\"next_sequence\":"+std::to_string(c.next)+",\"dropped\":"+std::to_string(c.dropped)+",\"first_sequence\":"+std::to_string(c.samples.empty()?c.next:c.samples.front().sequence)+",\"ranges\":[";
    for(auto& r:c.ranges) {if(out.back()!='[')out+=',';out+="{\"region\":"+controlQuote(r.region->id)+",\"offset\":"+std::to_string(r.offset)+",\"length\":"+std::to_string(r.length)+"}";} return out + "],\"first_frame\":" + (c.samples.empty() ? "null" : std::to_string(c.samples.front().frame))
      + ",\"last_frame\":" + (c.samples.empty() ? "null" : std::to_string(c.samples.back().frame))
      + ",\"last_copy_ns\":" + std::to_string(c.lastCopyNs) + ",\"max_copy_ns\":" + std::to_string(c.maxCopyNs)
      + ",\"copied_bytes\":" + std::to_string(c.copiedBytes) + ",\"transferred_bytes\":" + std::to_string(c.transferredBytes) + "}";
  }
  static std::string decimal(double v) { if(!std::isfinite(v)) return "null"; std::ostringstream out; out << std::setprecision(17) << v; return out.str(); }
  static double value(const std::vector<uint8_t>& b,size_t offset,const std::string& type,bool little) {
    unsigned width=type=="u8"?1:type=="u16"?2:4; uint32_t n=0;
    for(unsigned i=0;i<width;++i)n|=uint32_t(b[offset+i])<<((little?i:width-i-1)*8);
    if(type!="f32")return n; float f;std::memcpy(&f,&n,4);return f;
  }
#include "memory-query.inc.hpp"
  ControlResult execute(const std::string& method,const std::vector<std::string>& a,const std::string& owner,uint64_t current,uint64_t frame,const std::string& kind,bool paused) {
    sync(current);
    if(method.starts_with("memory.query.")) return query(method, a, owner);
    if(method == "memory.snapshot.changes") return changes(a, owner);
    if(method=="memory.snapshot.capture"||method=="memory.recorder.start") {
      bool recorder=method=="memory.recorder.start";uint32_t capacity=1,duration=0;
      if(a.size()<3)return {"invalid_request",{}};
      if(recorder && (!MemoryService::number(a[0],capacity)||!capacity||capacity>1024||!MemoryService::number(a[1],duration)||duration>1000000))return {"invalid_request",{}};
      if(!recorder && a[0]!="immediate" && a[0]!="boundary")return {"invalid_request",{}};
      if((recorder||a[0]=="boundary") && kind=="unavailable")return {"frame_boundary_unavailable",{}};
      if(!recorder && a[0]=="boundary" && paused)return {"paused_boundary_unavailable",{}};
      if(captures.size()>=32)return {"memory_handle_limit",{}};
      if(recorder) {unsigned count=0;for(auto& [id,c]:captures)if(c.recording)++count;if(count>=4)return {"recorder_limit",{}};}
      Capture c; c.owner=owner;c.epoch=current;c.id=++nextId;c.capacity=capacity;c.duration=duration;c.recording=recorder;
      auto error=validate(a,recorder?2:1,c.ranges,recorder?65536:16*1024*1024);if(!error.empty())return {error,{}};
      size_t bytes=0;for(auto& r:c.ranges)bytes+=r.length;
      if(bytes*capacity>budget-reserved())return {"memory_budget_exhausted",{}};
      c.active=recorder||a[0]=="boundary";c.pending=!recorder&&a[0]=="boundary";
      if(!c.active)sample(c,frame);
      captures.emplace(c.id,std::move(c));return {{},metadata(captures.at(nextId),kind)};
    }
    uint32_t id;if(a.empty()||!MemoryService::number(a[0],id))return {"invalid_request",{}};
    auto found=captures.find(id);if(found==captures.end()||found->second.owner!=owner)return {"stale_memory_handle",{}};
    auto& c=found->second;
    if(method == "memory.capture.freeze") {
      uint32_t sequence;
      if(a.size() != 2 || !MemoryService::number(a[1], sequence)) return {"invalid_request", {}};
      if(c.samples.empty() || sequence >= c.next) return {"sample_not_ready", {}};
      if(sequence < c.samples.front().sequence) return {"cursor_overwritten", {}};
      if(captures.size() >= 32) return {"memory_handle_limit", {}};
      size_t bytes = 0; for(const auto& r : c.ranges) bytes += r.length;
      if(bytes > budget - reserved()) return {"memory_budget_exhausted", {}};
      const auto started = std::chrono::steady_clock::now();
      Capture frozen; frozen.id = ++nextId; frozen.owner = owner; frozen.epoch = epoch; frozen.ranges = c.ranges; frozen.next = 1;
      frozen.samples.push_back(c.samples[sequence - c.samples.front().sequence]); frozen.samples.front().sequence = 0;
      frozen.lastCopyNs = frozen.maxCopyNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - started).count(); frozen.copiedBytes = bytes;
      auto [it, inserted] = captures.emplace(frozen.id, std::move(frozen)); return {{}, metadata(it->second, kind)};
    }
    if(method=="memory.capture.release") {if(a.size()!=1)return {"invalid_request",{}};captures.erase(found);return {{},"{\"released\":true}"};}
    if(method=="memory.recorder.stop") {if(a.size()!=1)return {"invalid_request",{}};c.active=false;c.pending=false;return {{},metadata(c,kind)};}
    if(method=="memory.capture.status") {if(a.size()!=1)return {"invalid_request",{}};return {{},metadata(c,kind)};}
    if(method=="memory.capture.read") {
      uint32_t sequence,range,offset,length;
      if(a.size()!=5||!MemoryService::number(a[1],sequence)||!MemoryService::number(a[2],range)||!MemoryService::number(a[3],offset)||!MemoryService::number(a[4],length)||!length||length>16384||range>=c.ranges.size())return {"invalid_request",{}};
      if(c.samples.empty())return {"sample_not_ready",{}};
      if(sequence<c.samples.front().sequence)return {"cursor_overwritten",{}};
      if(sequence>=c.next)return {"sample_not_ready",{}};
      auto& s=c.samples[sequence-c.samples.front().sequence];auto& b=s.bytes[range];
      if(offset>b.size()||length>b.size()-offset)return {"out_of_bounds",{}};
      c.transferredBytes += length;
      return {{},"{\"handle\":"+std::to_string(id)+",\"epoch\":"+std::to_string(current)+",\"sequence\":"+std::to_string(sequence)+",\"frame\":"+std::to_string(s.frame)+",\"range\":"+std::to_string(range)+",\"offset\":"+std::to_string(offset)+",\"hex\":"+controlQuote(hex(b,offset,length))+"}"};
    }
    // A query page scans at most 64 KiB and returns at most 256 matching values.
    // Cursor is byte-relative; immutable snapshots make continuation deterministic.
    if(method=="memory.snapshot.compare") {
      uint32_t other,range,cursor,limit;double expected;
      if(a.size()!=9||!MemoryService::number(a[1],other)||!MemoryService::number(a[2],range)||!MemoryService::number(a[6],cursor)||!MemoryService::number(a[7],limit)||!limit||limit>256)return {"invalid_request",{}};
      auto second=captures.find(other);if(second==captures.end()||second->second.owner!=owner)return {"stale_memory_handle",{}};
      auto& d=second->second;
      if(c.recording||d.recording||c.samples.empty()||d.samples.empty())return {"immutable_snapshot_required",{}};
      if(range>=c.ranges.size()||range>=d.ranges.size()||c.ranges[range].region!=d.ranges[range].region||c.ranges[range].offset!=d.ranges[range].offset||c.ranges[range].length!=d.ranges[range].length)return {"incompatible_snapshots",{}};
      unsigned width=a[3]=="u8"?1:a[3]=="u16"?2:(a[3]=="u32"||a[3]=="f32")?4:0;
      if(!width||(a[4]!="little"&&a[4]!="big")||cursor%width)return {"invalid_request",{}};
      if(a[5]!="changed"&&a[5]!="unchanged"&&a[5]!="increased"&&a[5]!="decreased"&&a[5]!="delta"&&a[5]!="equals")return {"invalid_request",{}};
      try {size_t end;expected=std::stod(a[8],&end);if(end!=a[8].size()||!std::isfinite(expected))return {"invalid_request",{}};} catch(...) {return {"invalid_request",{}};}
      auto& before=c.samples.front().bytes[range];auto& after=d.samples.front().bytes[range];if(cursor>before.size())return {"out_of_bounds",{}};
      size_t end=std::min(before.size(),size_t(cursor)+65536);unsigned count=0;std::string out="{\"semantics\":\"contents\",\"matches\":[";size_t i=cursor;
      for(;i+width<=end&&count<limit;i+=width) {
        double old=value(before,i,a[3],a[4]=="little"),now=value(after,i,a[3],a[4]=="little");
        bool match=matches(before,after,i,width,a[3],a[4]=="little",a[5],expected);
        if(match) {if(count++)out+=',';out+="{\"offset\":"+std::to_string(c.ranges[range].offset+i)+",\"old\":"+decimal(old)+",\"new\":"+decimal(now)+",\"old_hex\":"+controlQuote(hex(before,i,width))+",\"new_hex\":"+controlQuote(hex(after,i,width))+"}";}
      }
      return {{},out+"],\"next_cursor\":"+(i+width<=before.size()?std::to_string(i):"null")+"}"};
    }
    return {"unavailable",{}};
  }
};
