#pragma once
#include "machine-service.hpp"
#include "execution-service.hpp"
#include "capture-config.hpp"
struct ViewerService {
  std::string rspOwner, rdpOwner;
  CaptureConfig config;
  void loadRom(const std::string& path) { config.discover(path); }
  void reset() { rspOwner.clear(); rdpOwner.clear(); ares::Nintendo64::commandCapture.reset(); }
  void releaseOwner(const std::string& actor) {
    auto& c = ares::Nintendo64::commandCapture;
    if(rspOwner == actor) { c.rspFlush(); c.rspEnabled = false; rspOwner.clear(); }
    if(rdpOwner == actor) { c.rdpEnabled = false; rdpOwner.clear(); }
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& actor, uint64_t epoch, bool available, bool busy) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    auto& c = commandCapture;
    if(method == "n64.capture.control") {
      if(args.size() != 2) return {"invalid_request", {}};
      for(auto& a : args) if(a != "-1" && a != "0" && a != "1") return {"invalid_request", {}};
      if(args[0] != "-1" || args[1] != "-1") {
        if(busy || nall::GDB::server.hasClient()) return {"busy", {}};
        if((args[0] != "-1" && !rspOwner.empty() && rspOwner != actor && actor != "ui") ||
           (args[1] != "-1" && !rdpOwner.empty() && rdpOwner != actor && actor != "ui")) return {"busy", {}};
        // Validate both owners before changing either channel.
        if(args[0] != "-1") {
          bool enable = args[0] == "1";
          if(enable && !c.rspEnabled) { config.apply(); c.rspEnabled = true; c.detectRsp(); }
          else if(!enable) { c.rspFlush(); c.rspEnabled = false; }
          rspOwner = enable ? actor : "";
        }
        if(args[1] != "-1") { c.rdpEnabled = args[1] == "1"; rdpOwner = c.rdpEnabled ? actor : ""; }
      }
      MachineService::Object out;
      out.add("rsp", c.rspEnabled ? "true" : "false"); out.add("rdp", c.rdpEnabled ? "true" : "false");
      out.add("rsp_owner", rspOwner.empty() ? "null" : controlQuote(rspOwner)); out.add("rdp_owner", rdpOwner.empty() ? "null" : controlQuote(rdpOwner));
      out.add("rsp_mode", controlQuote(c.queueMode ? "RSPQ" : config.displayName)); out.add("rspq_supported", "true"); out.add("configuration", controlQuote(config.diagnostic)); out.number("capacity", c.Capacity); out.number("presented_frame", c.presentedFrame);
      return {{}, out.end()};
    }
    if(method != "n64.capture.rsp" && method != "n64.capture.rdp") return {"unavailable", {}};
    uint64_t generation, offset, limit;
    if(args.size() != 3 || !ExecutionService::number(args[0], generation) || !ExecutionService::number(args[1], offset) ||
       !ExecutionService::number(args[2], limit) || !limit || limit > 64) return {"invalid_request", {}};
    if(generation && generation != c.generation) return {"stale_capture", {}};
    bool isRsp = method == "n64.capture.rsp";
    auto retained = isRsp ? c.rspLatest.size() : c.rdpLatest.size();
    auto observed = isRsp ? c.rspCommitted : c.rdpCommitted;
    if(offset > retained) return {"out_of_bounds", {}};
    auto end = std::min<uint64_t>(retained, offset + limit);
    MachineService::Object out;
    out.number("epoch", epoch); out.number("generation", c.generation); out.add("kind", isRsp ? "\"rsp\"" : "\"rdp\"");
    out.add("mode", controlQuote(isRsp ? "live dispatch words, frozen semantic arguments and overhead segments" : "last committed VI-origin-swap command buffer; use next_frame=true for exact VI capture"));
    out.add("frame_kind", "\"vi_origin_swaps\""); out.number("presented_frame", c.presentedFrame);
    out.add("configured", !isRsp || c.sigPc >= 0 ? "true" : "false"); out.add("rspq_supported", "true");
    out.number("committed", observed); out.number("retained", retained); out.number("dropped", observed - retained);
    out.add("truncated", observed > retained ? "true" : "false"); out.number("capacity", c.Capacity); out.number("offset", offset);
    std::string rows = "[";
    for(uint64_t i = offset; i < end; ++i) {
      MachineService::Object row;
      if(isRsp) {
        auto& r = c.rspLatest[i];
        row.number("seq", r.seq); row.number("frame", r.frame); row.number("cycle", r.cycle); row.number("bytesIn", r.bytesIn); row.number("bytesOut", r.bytesOut);
        row.add("overlay", controlQuote(r.overlay.empty() ? "F3DEX2" : r.overlay)); row.add("command", controlQuote(r.command.empty() ? (r.overhead ? "Dispatch loop" : c.rspName(r.opcode)) : r.command));
        row.add("overhead", r.overhead ? "true" : "false");
        row.add("args", r.overhead ? "null" : controlQuote(r.args.empty() ? "word0=" + N64Service::hex(r.word0) + ", word1=" + N64Service::hex(r.word1) : r.args));
        row.add("words_truncated", r.wordsTruncated ? "true" : "false");
        std::string words = "[";
        for(unsigned w = 0; w < r.wordCount; ++w) { if(w) words += ','; words += controlQuote(N64Service::hex(r.words[w])); }
        row.add("words", words + ']');
      } else {
        auto& r = c.rdpLatest[i];
        row.number("seq", r.seq); row.number("frame", r.frame); row.hex("opcode", r.opcode); row.add("name", controlQuote(c.rdpName(r.opcode)));
        std::string description = CaptureRdpDecode::rdpCommandDescription(r.opcode, r.word0).data();
        std::replace(description.begin(),description.end(),'\n',' '); if(description.size() > 256) description = description.substr(0,253)+"...";
        row.add("description", controlQuote(description));
        row.hex("word0", r.word0); row.hex("word1", r.word1); row.number("wordCount", r.wordCount);
      }
      auto encoded = row.end();
      if(rows.size() + encoded.size() > 48000 && i > offset) { end = i; break; }
      if(i != offset) rows += ',';
      rows += encoded;
    }
    out.add("next_offset", end < retained ? std::to_string(end) : "null");
    out.add("entries", rows + ']'); return {{}, out.end()};
  }
};
