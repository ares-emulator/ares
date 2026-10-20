#pragma once
#include "execution-service.hpp"
#include "n64-service.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

// Native immutable pixels, independent of video credits and renderer lifetime.
struct FrameService {
  using Clock = std::chrono::steady_clock;
  struct Tag { uint64_t epoch = 0, frame = 0; };
  struct Image {
    std::vector<uint8_t> pixels;
    uint32_t width = 0, height = 0, origin = 0, depth = 0;
    uint64_t epoch = 0, frame = 0, sequence = 0, nonblack = 0;
    bool framebuffer = false;
  };
  struct Pin { uint64_t id; std::string owner; std::shared_ptr<const Image> image; Clock::time_point expires; };
  std::atomic<bool> enabled{false};
  std::mutex mutex;
  uint64_t epoch = 1, nextId = 1;
  std::unordered_map<const void*, Tag> submissions;
  std::shared_ptr<const Image> latest;
  std::vector<Pin> pins;
  static constexpr uint64_t maxPixels = 4194304;
  void submitted(const void* screen, uint64_t e, uint64_t f) {
    if(!enabled) return;
    std::lock_guard guard(mutex);
    if(submissions.size() < 8 || submissions.count(screen)) submissions[screen] = {e, f};
  }
  void forget(const void* screen) { std::lock_guard guard(mutex); submissions.erase(screen); }
  void reset(uint64_t e) {
    std::lock_guard guard(mutex); epoch = e; latest.reset(); pins.clear();
    // Keep submission tags: an already queued refresh must retain its old epoch.
  }
  void observe(const void* screen, const uint8_t* rgba, uint32_t width, uint32_t height, uint64_t sequence, uint64_t syntheticEpoch = 0, uint64_t syntheticFrame = 0) {
    if(!enabled || !width || !height || uint64_t(width) * height > maxPixels) return;
    std::lock_guard guard(mutex);
    Tag tag{syntheticEpoch, syntheticFrame};
    if(screen) { auto it = submissions.find(screen); if(it == submissions.end()) return; tag = it->second; }
    if(tag.epoch != epoch) return;
    auto image = std::make_shared<Image>();
    image->width = width; image->height = height; image->epoch = tag.epoch; image->frame = tag.frame; image->sequence = sequence;
    image->pixels.assign(rgba, rgba + uint64_t(width) * height * 4);
    latest = std::move(image);
  }
  void releaseOwner(const std::string& owner) {
    std::lock_guard guard(mutex); std::erase_if(pins, [&](const Pin& pin) { return pin.owner == owner; });
  }
  static ControlResult framebuffer(std::shared_ptr<const Image>& result, uint64_t e, uint64_t f) {
    using namespace ares::Nintendo64;
    auto& io = vi.io;
    uint32_t origin = io.dramAddress, width = io.width, depth = io.colorDepth;
    if(!width || (depth != 2 && depth != 3)) return {"vi_blank", {}};
    if(width > 2048) return {"vi_invalid", {}};
    uint32_t lines = io.vend > io.vstart ? (io.vend - io.vstart) >> 1 : 0;
    uint32_t scale = uint32_t(io.yscale) & 0xfff;
    uint32_t height = uint64_t(lines) * (scale ? scale : 0x400u) >> 10;
    if(!height) height = lines;
    if(!height) return {"vi_invalid", {}};
    height = std::min(height, 1024u);
    uint32_t bpp = depth == 2 ? 2 : 4;
    uint64_t fit = origin < rdram.ram.size ? (rdram.ram.size - origin) / (uint64_t(width) * bpp) : 0;
    height = std::min<uint64_t>(height, fit);
    if(!height) return {"vi_invalid", {}};
#if defined(VULKAN)
    if(vulkan.enable) vulkan.flush();
#endif
    auto image = std::make_shared<Image>();
    image->width = width; image->height = height; image->origin = origin; image->depth = depth;
    image->epoch = e; image->frame = f; image->framebuffer = true;
    image->pixels.resize(uint64_t(width) * height * 4);
    auto byte = [&](uint32_t off) { return uint8_t(rdram.ram.read<Byte>(off, RBusDevice::ARES_DEBUGGER)); };
    for(uint64_t n = 0; n < uint64_t(width) * height; ++n) {
      auto off = origin + n * bpp; auto* out = image->pixels.data() + n * 4;
      if(depth == 2) {
        uint16_t p = uint16_t(byte(off)) << 8 | byte(off + 1);
        uint8_t r = p >> 11, g = p >> 6 & 31, b = p >> 1 & 31;
        out[0] = r << 3 | r >> 2; out[1] = g << 3 | g >> 2; out[2] = b << 3 | b >> 2;
      } else { out[0] = byte(off); out[1] = byte(off + 1); out[2] = byte(off + 2); }
      out[3] = 255; image->nonblack += (out[0] | out[1] | out[2]) != 0;
    }
    result = std::move(image); return {};
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& owner, uint64_t e, uint64_t f, bool n64, const std::string& frameKind = "unavailable", bool paused = true) {
    std::shared_ptr<const Image> image;
    if(method == "video.begin") {
      if((args.size() != 1 && args.size() != 2) || (args[0] != "presented" && args[0] != "framebuffer")) return {"invalid_request", {}};
      if(args.size() == 2) {
        uint64_t expected;
        if(!ExecutionService::number(args[1], expected)) return {"invalid_request", {}};
        if(!paused || f != expected) return {"capture_frame_changed", {}};
      }
      enabled = true;
      if(args[0] == "framebuffer") {
        if(!n64) return {"unavailable", {}};
        auto result = framebuffer(image, e, f); if(!result.error.empty()) return result;
      }
    }
    std::lock_guard guard(mutex);
    std::erase_if(pins, [](const Pin& pin) { return Clock::now() >= pin.expires; });
    if(method == "video.begin") {
      if(e != epoch) return {"stale_epoch", {}};
      if(!image) image = latest;
      if(!image) return {"no_frame", {}};
      if(image->epoch != e) return {"stale_frame", {}};
      if(!image->framebuffer && image->width <= 1 && image->height <= 1) return {"invalid_frame", {}};
      if(pins.size() >= 2 || std::any_of(pins.begin(), pins.end(), [&](const Pin& pin) { return pin.owner == owner; })) return {"busy", {}};
      uint64_t id = nextId++; pins.push_back({id, owner, image, Clock::now() + std::chrono::seconds(30)});
      return {{}, "{\"snapshot_id\":" + std::to_string(id) + ",\"width\":" + std::to_string(image->width) + ",\"height\":" + std::to_string(image->height)
        + ",\"bytes\":" + std::to_string(image->pixels.size()) + ",\"epoch\":" + std::to_string(image->epoch) + ",\"frame\":" + std::to_string(image->frame)
        + ",\"frame_kind\":" + controlQuote(n64 ? "vi" : frameKind) + ",\"current_frame\":" + std::to_string(f)
        + ",\"sequence\":" + std::to_string(image->sequence) + ",\"source\":" + controlQuote(image->framebuffer ? "framebuffer" : "presented")
        + ",\"origin\":" + (image->framebuffer ? controlQuote(N64Service::hex(image->origin)) : "null") + ",\"color_depth\":" + std::to_string(image->depth)
        + ",\"nonblack_pixels\":" + (image->framebuffer ? std::to_string(image->nonblack) : "null") + ",\"format\":\"rgba8\"}"};
    }
    uint64_t id, offset = 0, length = 0;
    if(method != "video.read" && method != "video.release") return {"unavailable", {}};
    if(args.size() != (method == "video.read" ? 3 : 1) || !ExecutionService::number(args[0], id) || !id ||
       (method == "video.read" && (!ExecutionService::number(args[1], offset) || !ExecutionService::number(args[2], length) || !length || length > 16384))) return {"invalid_request", {}};
    auto pin = std::find_if(pins.begin(), pins.end(), [&](const Pin& pin) { return pin.id == id; });
    if(pin == pins.end()) return {"stale_snapshot", {}};
    if(pin->owner != owner) return {"busy", {}};
    if(method == "video.release") { pins.erase(pin); return {{}, "{\"released\":true}"}; }
    auto& data = pin->image->pixels;
    if(offset >= data.size() || length > data.size() - offset) return {"out_of_bounds", {}};
    std::string hex; hex.reserve(length * 2); static const char digits[] = "0123456789abcdef";
    for(uint64_t n = offset; n < offset + length; ++n) { hex += digits[data[n] >> 4]; hex += digits[data[n] & 15]; }
    return {{}, "{\"snapshot_id\":" + std::to_string(id) + ",\"offset\":" + std::to_string(offset) + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(hex) + "}"};
  }
};
