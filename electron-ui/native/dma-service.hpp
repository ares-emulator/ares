#pragma once
#include "n64-service.hpp"
#include "execution-service.hpp"
struct DmaService {
  bool available = false;
  ControlResult execute(const std::vector<std::string>& args) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    uint64_t since, limit, low, high;
    if(args.size() != 4 || !ExecutionService::number(args[0], since) || !ExecutionService::number(args[1], limit) || !limit || limit > 16 ||
       !ExecutionService::number(args[2], low) || !ExecutionService::number(args[3], high) || low >= high || high > 0x100000000ull) return {"invalid_request", {}};
    dmaProvenance.enable();
    std::vector<const DmaProvenance::Record*> records;
    for(auto& r : dmaProvenance.records) if(r.id > since) records.push_back(&r);
    std::sort(records.begin(), records.end(), [](auto* a, auto* b) { return a->id < b->id; });
    std::string out = "{\"entries\":["; uint64_t last = since, count = 0;
    auto hex = [](uint64_t n) { return controlQuote(N64Service::hex(n)); };
    for(auto* r : records) {
      if(count >= limit) break;
      if(count++) out += ',';
      bool candidate = false; std::string ranges = "[";
      for(auto& range : r->ranges) {
        if(ranges.size() > 1) ranges += ',';
        ranges += "{\"dram_address\":" + hex(range.address) + ",\"length\":" + std::to_string(range.length) + "}";
        if(uint64_t(range.address) < high && low < uint64_t(range.address) + range.length) candidate = true;
      }
      out += "{\"id\":" + std::to_string(r->id) + ",\"epoch\":" + std::to_string(r->epoch) + ",\"frame\":" + std::to_string(r->frame)
        + ",\"cart_address\":" + hex(r->cartAddress) + ",\"dram_address\":" + hex(r->dramAddress) + ",\"requested_length\":" + std::to_string(r->requestedLength)
        + ",\"from_rom\":" + (r->fromRom ? "true" : "false") + ",\"rom_offset\":" + (r->fromRom ? hex(r->romOffset) : "null") + ",\"cpu_pc\":" + hex(r->pc)
        + ",\"bytes_attempted\":" + std::to_string(r->bytesAttempted) + ",\"bytes_written\":" + (r->writesConfirmed ? std::to_string(r->bytesWritten) : "null")
        + ",\"writes_confirmed\":" + (r->writesConfirmed ? "true" : "false") + ",\"ranges_truncated\":" + (r->rangesTruncated ? "true" : "false")
        + ",\"ranges\":" + ranges + "],\"code_candidate\":" + (candidate && r->fromRom && r->writesConfirmed ? "true" : "false") + "}";
      last = r->id;
    }
    return {{}, out + "],\"next_since_id\":" + std::to_string(last) + ",\"more\":" + (records.size() > count ? "true" : "false")
      + ",\"capacity\":4096,\"dropped\":" + std::to_string(dmaProvenance.dropped) + ",\"epoch\":" + std::to_string(dmaProvenance.epoch)
      + ",\"history_complete\":" + (dmaProvenance.historyComplete && !dmaProvenance.dropped ? "true" : "false") + ",\"recording_from_epoch_start\":" + (dmaProvenance.historyComplete ? "true" : "false") + ",\"enabled_at_frame\":" + std::to_string(dmaProvenance.enabledAtFrame)
      + ",\"code_range\":{\"start\":" + hex(low) + ",\"end\":" + hex(high) + "},\"note\":\"PI cartridge-to-RDRAM only. Code candidates are not proof of overlays. Unconfirmed ranges are attempted bus writes; decompressed/generated code is not captured.\"}"};
  }
};
