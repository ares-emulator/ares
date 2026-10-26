#include "../../desktop-ui.hpp"
#include "hash_chd.hpp"

namespace {

#if defined(ARES_ENABLE_CHD)
struct TrackHandle {
  Decode::CHD chd;
  void* fallback = nullptr;
  rc_hash_cdreader_t defaultReader{};
  u32 firstSector = 0;
  u32 lastSector = 0;
  u32 dataOffset = 0;
  u32 dataSize = 2048;
  bool isChd = false;
};

auto dataIndex(const Decode::CHD::Track& track) -> const Decode::CHD::Index* {
  for(auto& index : track.indices) {
    if(index.number == 1 && index.lba >= 0 && index.end >= index.lba) return &index;
  }
  return nullptr;
}

auto openTrack(const char* path, u32 requestedTrack, const rc_hash_iterator_t* iterator) -> void* {
  if(!path) return nullptr;
  auto handle = std::make_unique<TrackHandle>();

  if(!string{path}.iendsWith(".chd")) {
    rc_hash_get_default_cdreader(&handle->defaultReader);
    if(!handle->defaultReader.open_track_iterator) return nullptr;
    handle->fallback = handle->defaultReader.open_track_iterator(path, requestedTrack, iterator);
    return handle->fallback ? handle.release() : nullptr;
  }

  if(!handle->chd.load(path)) return nullptr;
  const Decode::CHD::Track* selectedTrack = nullptr;
  if(requestedTrack == RC_HASH_CDTRACK_FIRST_DATA) {
    for(auto& track : handle->chd.tracks) {
      if(track.type == "AUDIO" || !dataIndex(track)) continue;
      selectedTrack = &track;
      break;
    }
  } else if(requestedTrack == RC_HASH_CDTRACK_LAST) {
    for(auto index = (s32)handle->chd.tracks.size() - 1; index >= 0; --index) {
      if(dataIndex(handle->chd.tracks[index])) {
        selectedTrack = &handle->chd.tracks[index];
        break;
      }
    }
  } else if(requestedTrack == RC_HASH_CDTRACK_LARGEST) {
    u32 largest = 0;
    for(auto& track : handle->chd.tracks) {
      auto* index = dataIndex(track);
      if(!index || index->sectorCount() <= largest) continue;
      largest = index->sectorCount();
      selectedTrack = &track;
    }
  } else {
    for(auto& track : handle->chd.tracks) {
      if(track.number == requestedTrack && dataIndex(track)) {
        selectedTrack = &track;
        break;
      }
    }
  }

  if(!selectedTrack) return nullptr;
  auto* index = dataIndex(*selectedTrack);
  handle->firstSector = index->lba;
  handle->lastSector = index->end;
  if(selectedTrack->type == "AUDIO") {
    handle->dataSize = 2352;
  } else if(selectedTrack->type.find("MODE2")) {
    handle->dataOffset = 24;
  } else {
    handle->dataOffset = 16;
  }
  handle->isChd = true;
  return handle.release();
}

auto readSector(void* trackHandle, u32 sector, void* buffer, size_t requestedBytes) -> size_t {
  auto* handle = (TrackHandle*)trackHandle;
  if(!handle || !buffer || !requestedBytes) return 0;
  if(!handle->isChd) return handle->defaultReader.read_sector(handle->fallback, sector, buffer, requestedBytes);
  if(sector < handle->firstSector || sector > handle->lastSector) return 0;

  auto* out = (u8*)buffer;
  size_t total = 0;
  while(requestedBytes && sector <= handle->lastSector) {
    auto raw = handle->chd.read(sector++);
    if(raw.empty()) break;
    auto offset = raw.size() > handle->dataSize ? handle->dataOffset : 0;
    if(offset >= raw.size()) break;
    auto available = min<size_t>(raw.size() - offset, handle->dataSize);
    auto count = min(requestedBytes, available);
    memory::copy(out, raw.data() + offset, count);
    out += count;
    total += count;
    requestedBytes -= count;
  }
  return total;
}

auto firstTrackSector(void* trackHandle) -> u32 {
  auto* handle = (TrackHandle*)trackHandle;
  if(!handle) return 0;
  if(!handle->isChd) return handle->defaultReader.first_track_sector(handle->fallback);
  return handle->firstSector;
}

auto closeTrack(void* trackHandle) -> void {
  auto* handle = (TrackHandle*)trackHandle;
  if(!handle) return;
  if(!handle->isChd) handle->defaultReader.close_track(handle->fallback);
  delete handle;
}
#endif

}

namespace RA::Platform {

auto configureChdHashCallbacks(rc_hash_callbacks_t& callbacks) -> void {
#if defined(ARES_ENABLE_CHD)
  callbacks.cdreader.open_track_iterator = openTrack;
  callbacks.cdreader.read_sector = readSector;
  callbacks.cdreader.first_track_sector = firstTrackSector;
  callbacks.cdreader.close_track = closeTrack;
#else
  (void)callbacks;
#endif
}

}
