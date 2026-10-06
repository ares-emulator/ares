#pragma once

#include <nall/cd.hpp>
#include <nall/file.hpp>
#include <nall/string.hpp>
#include <nall/decode/cue.hpp>
#if defined(ARES_ENABLE_CHD)
#include <nall/decode/chd.hpp>
#endif
#include <nall/decode/wav.hpp>
#include <nall/decode/disc-archive.hpp>
#include <limits>
#include <utility>
#include <vector>

namespace nall::vfs {

struct cdrom : file {
  ~cdrom() {
    _thread.join();
  }

  static auto open(const string& location, const string& pathWithinArchive, string* error = nullptr) -> std::shared_ptr<cdrom> {
    struct enable_make_shared : cdrom { using cdrom::cdrom; };
    auto instance = std::make_shared<enable_make_shared>();

    if (location.iendsWith(".mmi")) {
      instance->_archive = std::make_unique<Decode::ZIPArchive>();
      if (!instance->_archive->open(location)) {
        if(error) *error = instance->_archive->error();
        return {};
      }

      maybe<Decode::Archive::File> compressedFile = instance->_archive->findFile(pathWithinArchive);
      if (compressedFile && instance->loadCue(location, instance->_archive.get(), &compressedFile.get(), error)) return instance;
    }

    return {};
  }

  static auto open(const string& location, string* error = nullptr) -> std::shared_ptr<cdrom> {
    struct enable_make_shared : cdrom { using cdrom::cdrom; };
    auto instance = std::make_shared<enable_make_shared>();
    if(location.iendsWith(".cue") && instance->loadCue(location, nullptr, nullptr, error)) return instance;
#if defined(ARES_ENABLE_CHD)
    if(location.iendsWith(".chd") && instance->loadChd(location)) return instance;
#endif
    if(location.iendsWith(".zip") || location.iendsWith(".7z")) {
      Decode::DiscArchive source;
      if(source.open(location)) {
        auto descriptor = source.descriptor;
        instance->_archive = std::move(source.archive);
        if(instance->loadCue(location, instance->_archive.get(), &descriptor, error)) return instance;
      } else if(error) {
        *error = source.error();
      }
    }
    return {};
  }

  auto writable() const -> bool override { return false; }
  auto data() const -> const u8* override { wait(size()); return _image.data(); }
  auto data() -> u8* override { wait(size()); return _image.data(); }
  auto size() const -> u64 override { return _image.size(); }
  auto offset() const -> u64 override { return _offset; }

  auto resize(u64 size) -> bool override {
    //unsupported
    return false;
  }

  auto seek(s64 offset, index mode) -> void override {
    if(mode == index::absolute) _offset  = (u64)offset;
    if(mode == index::relative) _offset += (s64)offset;
  }

  auto read() -> u8 override {
    if(_offset >= _image.size()) return 0x00;
    wait(_offset);
    return _image[_offset++];
  }

  auto write(u8 data) -> void override {
    //CD-ROMs are read-only; but allow writing anyway if needed, since the image is in memory
    if(_offset >= _image.size()) return;
    wait(_offset);
    _image[_offset++] = data;
  }

  auto wait(u64 offset) const -> void {
    bool force = false;
    if(offset >= _image.size()) {
      offset = _image.size() - 1;
      force = true;
    }
    //subchannel data is always loaded
    if(offset % 2448 < 2352 || force) {
      while(offset + 1 > _loadOffset) usleep(1);
    }
  }

private:
  auto loadCue(const string& cueLocation, Decode::Archive* archive, const Decode::Archive::File* compressedFile, string* error) -> bool {
    constexpr u64 MaximumDiscImageSize = 1ull << 30;
    auto failLayout = [&](const string& message) -> bool {
      if(error) *error = message;
      if(archive) archive->close();
      return false;
    };
    auto cuesheet = std::make_shared<Decode::CUE>();
    if(!cuesheet->load(cueLocation, archive, compressedFile)) {
      if(error) *error = cuesheet->error();
      return false;
    }
    const bool archived = archive != nullptr && compressedFile != nullptr;

    CD::Session session;
    session.leadIn.lba = -(CD::LeadInSectors + CD::Track1Pregap);
    session.leadIn.end = -CD::Track1Pregap - 1;

    s64 lbaFileBase = 0;
    s64 lbaIndex = 0;
    for(auto& file : cuesheet->files) {
      for(auto& track : file.tracks) {
        session.tracks[track.number].control = track.type == "audio" ? 0b0000 : 0b0100;
        if(track.pregap) {
          if(*track.pregap < 0 || lbaFileBase > std::numeric_limits<s32>::max() - *track.pregap)
            return failLayout("CUE pregap exceeds the supported disc layout.");
          lbaFileBase += *track.pregap;
        }
        for(auto& index : track.indices) {
          if(index.lba >= 0) {
            auto indexLba = lbaFileBase + index.lba;
            auto indexEnd = lbaFileBase + index.end;
            if(indexLba < 0 || indexEnd < -1 || indexLba > std::numeric_limits<s32>::max()
            || indexEnd > std::numeric_limits<s32>::max())
              return failLayout("CUE index exceeds the supported disc layout.");
            session.tracks[track.number].indices[index.number].lba = (s32)indexLba;
            session.tracks[track.number].indices[index.number].end = (s32)indexEnd;
            if(index.number == 0 && track.pregap) {
              session.tracks[track.number].indices[index.number].lba -= track.pregap();
              session.tracks[track.number].indices[index.number].end -= track.pregap();
            }
          } else {
            // insert gap
            session.tracks[track.number].indices[index.number].lba = lbaIndex;
            if(index.number == 0)
              session.tracks[track.number].indices[index.number].end = lbaIndex + track.pregap() - 1;
            else
              session.tracks[track.number].indices[index.number].end = lbaIndex + track.postgap() - 1;
          }
          lbaIndex = session.tracks[track.number].indices[index.number].end + 1;
        }
        if(track.postgap) {
          if(*track.postgap < 0 || lbaFileBase > std::numeric_limits<s32>::max() - *track.postgap)
            return failLayout("CUE postgap exceeds the supported disc layout.");
          lbaFileBase += *track.postgap;
        }
      }
      lbaFileBase = lbaIndex;
    }
    if(lbaFileBase < 0 || lbaFileBase > std::numeric_limits<s32>::max() - CD::LeadOutSectors)
      return failLayout("CUE exceeds the supported disc layout.");
    session.leadOut.lba = lbaFileBase;
    session.leadOut.end = lbaFileBase + CD::LeadOutSectors - 1;

    // determine track and index ranges
    session.firstTrack = 0xff;
    for(u32 track : range(100)) {
      if(!session.tracks[track]) continue;
      if(session.firstTrack > 99) session.firstTrack = track;
      // find first index
      for(u32 indexID : range(100)) {
        auto& index = session.tracks[track].indices[indexID];
        if(index) { session.tracks[track].firstIndex = indexID; break; }
      }
      // find last index
      for(u32 indexID : reverse(range(100))) {
        auto& index = session.tracks[track].indices[indexID];
        if(index) { session.tracks[track].lastIndex = indexID; break; }
      }
      session.lastTrack = track;
    }

    auto imageSectors = (u64)CD::LeadInSectors + (u64)CD::LBAtoABA((s32)lbaFileBase) + CD::LeadOutSectors;
    if(imageSectors > MaximumDiscImageSize / 2448)
      return failLayout("CUE disc image exceeds the 1 GiB in-memory safety limit.");
    _image.resize(2448ull * imageSectors);

    //preload subchannel data
    if (compressedFile != nullptr) {
      auto subFile = archive->findFile({ Location::notsuffix(compressedFile->name), ".sub" });
      if (subFile) {
        loadSub(cueLocation, archive, &subFile.get(), session);
      } else {
        loadSub({ Location::notsuffix(cueLocation), ".sub" }, nullptr, nullptr, session);
      }
    } else {
      loadSub({ Location::notsuffix(cueLocation), ".sub" }, archive, compressedFile, session);
    }

    auto loadError = std::make_shared<string>();
    auto loadImage = [this, archive, archived, cueLocation, cuesheet, loadError]() -> bool {
      auto fail = [&](const string& message) -> bool {
        *loadError = message;
        if(archived) archive->close();
        _loadOffset = _image.size();
        return false;
      };

      s32 lbaFileBase = 0;
      for(auto& file : cuesheet->files) {
        bool usingFileBuffer = false;
        size_t fileDataReadPos = 0;
        file_buffer fileBuffer;
        std::vector<u8> rawDataBuffer;
        std::span<const u8> rawDataView;
        if(archived) {
          auto filePathInArchive = Decode::Archive::resolveMemberName(file.archiveFolder, file.name);
          if(!filePathInArchive) return fail({"Unsafe CUE member reference: ", file.name});

          auto fileEntry = archive->findFile(*filePathInArchive);
          if(!fileEntry) return fail(archive->error());
          if(archive->isDataUncompressed(*fileEntry)) {
            rawDataView = archive->dataViewIfUncompressed(*fileEntry);
          } else {
            rawDataBuffer = archive->extract(*fileEntry);
            rawDataView = {rawDataBuffer.data(), rawDataBuffer.size()};
          }
          if(rawDataView.size() != fileEntry->size) {
            auto message = archive->error();
            if(!message) message = {"Failed to read archived disc member: ", fileEntry->name};
            return fail(message);
          }
        } else {
          auto location = string{Location::path(cueLocation), file.name};
          fileBuffer = nall::file::open(location, nall::file::mode::read);
          if(!fileBuffer) return fail({"Failed to open CUE member: ", location});
          usingFileBuffer = true;
        }
        if(file.type == "wave") {
          if(usingFileBuffer) fileBuffer.seek(44); //skip RIFF header
          else fileDataReadPos = 44;
        }
        for(auto& track : file.tracks) {
          if(track.pregap) lbaFileBase += track.pregap();
          for(auto& index : track.indices) {
            if(index.lba < 0) continue; // ignore gaps (not in file)
            for(s32 sector : range(index.sectorCount())) {
              auto lba = lbaFileBase + index.lba + sector;
              auto offset = 2448ull * (CD::LeadInSectors + (u64)CD::LBAtoABA(lba));
              auto length = track.sectorSize();
              if(!length) return fail({"Unsupported CUE track type: ", track.type});
              if(offset > _image.size() || _image.size() - offset < 2448)
                return fail("CUE track layout exceeds the allocated disc image.");
              auto target = _image.data() + offset;

              if(length == 2048) {
                //ISO: generate header + parity data
                memory::assign(target + 0,  0x00, 0xff, 0xff, 0xff, 0xff, 0xff);  //sync
                memory::assign(target + 6,  0xff, 0xff, 0xff, 0xff, 0xff, 0x00);  //sync
                auto msf = CD::MSF::fromABA(CD::LBAtoABA(lba));
                target[12] = BCD::encode(msf.minute);
                target[13] = BCD::encode(msf.second);
                target[14] = BCD::encode(msf.frame);
                target[15] = 0x01;  // mode
                if(usingFileBuffer) {
                  fileBuffer.read({target + 16, length});
                } else {
                  if(fileDataReadPos > rawDataView.size() || rawDataView.size() - fileDataReadPos < length)
                    return fail({"Archived disc member is shorter than its CUE layout: ", file.name});
                  memory::copy(target + 16, length, rawDataView.data() + fileDataReadPos, length);
                  fileDataReadPos += length;
                }
                CD::RSPC::encodeMode1({target, 2352});
              } else if(length == 2352) {
                //BIN + WAV: direct copy
                if(usingFileBuffer) {
                  fileBuffer.read({target, length});
                } else {
                  if(fileDataReadPos > rawDataView.size() || rawDataView.size() - fileDataReadPos < length)
                    return fail({"Archived disc member is shorter than its CUE layout: ", file.name});
                  memory::copy(target, length, rawDataView.data() + fileDataReadPos, length);
                  fileDataReadPos += length;
                }
              }
              _loadOffset = offset + 2448;
            }
          }
          if(track.postgap) lbaFileBase += track.postgap();
        }
        lbaFileBase += file.sectorCount();
      }

      if(archived) archive->close();
      _loadOffset = _image.size();
      return true;
    };

    // Archived discs are decoded synchronously so corrupt/encrypted members
    // cannot surface as a successful load followed by silent zero sectors.
    if(archived) {
      if(!loadImage()) {
        if(error) *error = *loadError;
        return false;
      }
    } else {
      _thread = thread::create([loadImage = std::move(loadImage)](uintptr) mutable -> void {
        loadImage();
      });
    }

    return true;
  }
#if defined(ARES_ENABLE_CHD)
  auto loadChd(const string& location) -> bool {
    auto chd = std::make_shared<Decode::CHD>();
    if(!chd->load(location)) return false;

    CD::Session session;
    session.leadIn.lba = -(CD::LeadInSectors + CD::Track1Pregap);
    session.leadIn.end = -CD::Track1Pregap - 1;

    s32 lbaIndex = 0;
    for(auto& track : chd->tracks) {
      session.tracks[track.number].control = track.type == "AUDIO" ? 0b0000 : 0b0100;
      for(auto& index : track.indices) {
        session.tracks[track.number].indices[index.number].lba = index.lba;
        session.tracks[track.number].indices[index.number].end = index.end;
        lbaIndex = session.tracks[track.number].indices[index.number].end + 1;
      }
    }

    session.leadOut.lba = lbaIndex;
    session.leadOut.end = lbaIndex + CD::LeadOutSectors - 1;

    // determine track and index ranges
    session.firstTrack = 0xff;
    for(u32 track : range(100)) {
      if(!session.tracks[track]) continue;
      if(session.firstTrack > 99) session.firstTrack = track;
      // find first index
      for(u32 indexID : range(100)) {
        auto& index = session.tracks[track].indices[indexID];
        if(index) { session.tracks[track].firstIndex = indexID; break; }
      }
      // find last index
      for(u32 indexID : reverse(range(100))) {
        auto& index = session.tracks[track].indices[indexID];
        if(index) { session.tracks[track].lastIndex = indexID; break; }
      }
      session.lastTrack = track;
    }

    _image.resize(2448ull * (CD::LeadInSectors + CD::LBAtoABA(lbaIndex) + CD::LeadOutSectors));

    //preload subchannel data
    loadSub({Location::notsuffix(location), ".sub"}, nullptr, nullptr, session);

    //load user data on separate thread
    _thread = thread::create(
    [this, chd = std::move(chd)](uintptr) -> void {

    s32 lbaFileBase = 0;
    for(auto& track : chd->tracks) {
      for(auto& index : track.indices) {
        for(s32 sector : range(index.sectorCount())) {
          auto lba = index.lba + sector;
          auto offset = 2448ull * (CD::LeadInSectors + (u64)CD::LBAtoABA(lba));
          auto target = _image.data() + offset;
          auto sectorData = chd->read(lbaFileBase);
          if(sectorData.size() == 2048) {
            //ISO: generate header + parity data
            memory::assign(target + 0,  0x00, 0xff, 0xff, 0xff, 0xff, 0xff);  //sync
            memory::assign(target + 6,  0xff, 0xff, 0xff, 0xff, 0xff, 0x00);  //sync
            auto msf = CD::MSF::fromABA(CD::LBAtoABA(lba));
            target[12] = BCD::encode(msf.minute);
            target[13] = BCD::encode(msf.second);
            target[14] = BCD::encode(msf.frame);
            target[15] = 0x01;  // mode
            memory::copy(target + 16, 2048, sectorData.data(), sectorData.size());
            CD::RSPC::encodeMode1({target, 2352});
          } else {
            memory::copy(target, 2352, sectorData.data(), sectorData.size());
          }
          lbaFileBase++;
          _loadOffset = offset + 2448;
        }
      }
    }
    _loadOffset = _image.size();

    });

    return true;
  }
#endif

  void loadSub(const string& location, const Decode::Archive* archive, const Decode::Archive::File* compressedFile, CD::Session& session) {
    auto subchannel = session.encode((u32)abs(session.leadIn.lba) + (u32)session.leadOut.end + 1);
    const u64 overlayStartSectors = (u64)CD::LeadInSectors + (u64)CD::Track1Pregap;
    const u64 overlayStartBytes   = overlayStartSectors * 96;
    if(archive != nullptr && compressedFile != nullptr) {
      auto rawDataBuffer = archive->extract(*compressedFile);
      if(!rawDataBuffer.empty() && overlayStartBytes < subchannel.size()) {
        auto target = subchannel.data() + overlayStartBytes;
        auto maxLen = subchannel.size() - overlayStartBytes;
        auto length = std::min<u64>(maxLen, rawDataBuffer.size());
        memory::copy(target, (s64)length, rawDataBuffer.data(), length);
      }
    } else {
      auto overlay = nall::file::read(location);
      if(!overlay.empty() && overlayStartBytes < subchannel.size()) {
        auto target = subchannel.data() + overlayStartBytes;
        auto maxLen = subchannel.size() - overlayStartBytes;
        auto length = std::min<u64>(maxLen, overlay.size());
        memory::copy(target, length, overlay.data(), length);
      }
    }

    const u64 sectorCount = subchannel.size() / 96;
    for(u64 sector : range(sectorCount)) {
      auto* source = subchannel.data() + sector * 96;
      auto* target = _image.data() + sector * 2448 + 2352;
      memory::copy(target, 96, source, 96);
    }

    // Diagnostic: decode what we generated and dump it
    CD::Session finalSession;
    finalSession.decode(subchannel, 96);
    print(finalSession.serialize());
  }

  std::vector<u8> _image;
  u64 _offset = 0;
  atomic<u64> _loadOffset = 0;
  thread _thread;
  std::unique_ptr<Decode::Archive> _archive;
};

}
