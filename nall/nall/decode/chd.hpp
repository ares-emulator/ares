#pragma once

#include <nall/file.hpp>
#include <nall/maybe.hpp>
#include <nall/string.hpp>
#if defined(ARES_ENABLE_CHD)
#include <libchdr/chd.h>
#endif

namespace nall::Decode {

struct CHD {
  ~CHD();
  enum class Subchannel { None, RW, RWRaw };
  struct Index {
    auto sectorCount() const -> u32;

    u8 number = 0xff; //00-99
    s32 lba = -1;
    s32 end = -1;
    s32 chd_lba = -1;
  };

  struct Track {
    auto sectorCount() const -> u32;

    u8 number = 0xff; //01-99
    string type;
    Subchannel subchannel = Subchannel::None;
    std::vector<Index> indices;
    maybe<s32> pregap;
    maybe<s32> postgap;
  };

  auto load(const string& location) -> bool;
  auto read(u32 sector) const -> std::vector<u8>;
  auto readSubchannel(u32 sector) const -> std::vector<u8>;
  auto sectorCount() const -> u32;

  std::vector<Track> tracks;
private:
  auto readFrame(u32 sector) const -> const u8*;
  file_buffer fp;
  chd_file* chd = nullptr;
  static constexpr int chd_sector_size = 2352 + 96;
  size_t chd_hunk_size;
  mutable std::vector<u8> chd_hunk_buffer;
  mutable int chd_current_hunk = -1;
};

inline CHD::~CHD() {
  if (chd != nullptr) {
     chd_close(chd);
  }
}

inline auto CHD::load(const string& location) -> bool {
  fp = file::open(location, file::mode::read);
  if(!fp) {
    print("CHD: Failed to open ", location, "\n");
    return false;
  }

  chd_error err = chd_open_file(fp.handle(), CHD_OPEN_READ, nullptr, &chd);
  if (err != CHDERR_NONE) {
    print("CHD: Failed to open ", location, ": ", chd_error_string(err), "\n");
    return false;
  }

  const chd_header* header = chd_get_header(chd);
  chd_hunk_size = header->hunkbytes;

  if ((chd_hunk_size % chd_sector_size) != 0) {
    print("CHD: hunk size (", chd_hunk_size, ") is not a multiple of ", chd_sector_size, "\n");
    return false;
  }

  chd_hunk_buffer.resize(chd_hunk_size);
  u32 disc_lba = 0;
  u32 chd_lba = 0;

  // Fetch track structure
  while(true) {
    char metadata[256];
    char type[256];
    char subtype[256];
    char pgtype[256];
    char pgsub[256];
    u32 metadata_size;

    int track_no;
    int frames;
    int pregap_frames = 0;
    int postgap_frames = 0;

    // First, attempt to fetch CDROMv2 metadata
    err = chd_get_metadata(chd, CDROM_TRACK_METADATA2_TAG, tracks.size(), metadata, sizeof(metadata), &metadata_size, nullptr, nullptr);
    if (err == CHDERR_NONE) {
      if (std::sscanf(metadata, CDROM_TRACK_METADATA2_FORMAT, &track_no, type, subtype, &frames, &pregap_frames, pgtype, pgsub, &postgap_frames) != 8) {
        print("CHD: Invalid track v2 metadata: ", metadata,  "\n");
        return false;
      }
    } else {
      // That failed, so try to fetch CDROM (old) metadata
      err = chd_get_metadata(chd, CDROM_TRACK_METADATA_TAG, tracks.size(), metadata, sizeof(metadata),  &metadata_size, nullptr, nullptr);
      if (err != CHDERR_NONE) {
        // Both meta-data types failed to fetch, so assume there are no further tracks
        break;
      }

      if (std::sscanf(metadata, CDROM_TRACK_METADATA_FORMAT, &track_no, type, subtype, &frames) != 4) {
        print("CHD: Invalid track metadata: ", metadata, "\n");
        return false;
      }
    }

    // We currently only support RAW and audio tracks; log an error and exit if we see anything different
    auto typeStr = string{type};
    if (!(typeStr.find("_RAW") || typeStr.find("AUDIO") || typeStr.find("MODE1"))) {
      print("CHD: Unsupported track type: ", type, "\n");
      return false;
    }

    const bool pregap_in_file = (pregap_frames > 0 && pgtype[0] == 'V');
    const int track1_pregap = (track_no == 1 && !pregap_in_file) ? 2 * 75 : 0;

    // Add the new track
    Track track;
    track.number = track_no;
    track.type = type;
    track.subchannel = string{subtype} == "RW" ? Subchannel::RW
      : string{subtype} == "RW_RAW" ? Subchannel::RWRaw : Subchannel::None;
    track.pregap = pregap_frames;
    track.postgap = postgap_frames;

    // index0 = Pregap
    if (pregap_frames > 0 || track1_pregap > 0) {
      Index index;
      index.number = 0;

      if (pregap_in_file) {
        index.lba = disc_lba;
        index.end = disc_lba + pregap_frames - 1;
        index.chd_lba = chd_lba;

        if (pregap_frames > frames) {
          print("CHD: pregap length ", pregap_frames, " exceeds track length ", frames, "\n");
          return false;
        }

        disc_lba += pregap_frames;
        chd_lba  += pregap_frames;
        frames   -= pregap_frames;

        track.indices.push_back(index);
      } else if(track_no == 1 && track1_pregap) {
        index.lba = -track1_pregap;
        index.end = -1;
        index.chd_lba = -1;
        track.indices.push_back(index);
      }
    }

    // index1 = track data
    {
      Index index;
      index.number = 1;
      index.lba = disc_lba;
      index.end = disc_lba + frames - 1;
      index.chd_lba = chd_lba;
      track.indices.push_back(index);
      disc_lba += frames;
      chd_lba += frames;

      // chdman pads each track to a 4-frame boundary
      chd_lba = (chd_lba + 3) / 4 * 4;
    }

    // index2 = postgap
    if (postgap_frames > 0) {
      Index index;
      index.number = 2;
      index.lba = disc_lba;
      index.end = disc_lba + postgap_frames - 1;
      index.chd_lba = -1;
      track.indices.push_back(index);
      disc_lba += postgap_frames;
    }

    tracks.push_back(track);
  }

  return true;
}

inline auto CHD::readFrame(u32 sector) const -> const u8* {
  u64 address = u64(sector) * chd_sector_size;
  u32 hunk = address / chd_hunk_size;
  u32 offset = address % chd_hunk_size;
  if(hunk != chd_current_hunk) {
    chd_current_hunk = -1;
    if(chd_read(chd, hunk, chd_hunk_buffer.data()) != CHDERR_NONE) return nullptr;
    chd_current_hunk = hunk;
  }
  return chd_hunk_buffer.data() + offset;
}

inline auto CHD::readSubchannel(u32 sector) const -> std::vector<u8> {
  for(auto& track : tracks) {
    for(auto& index : track.indices) {
      if(sector < index.lba || sector > index.end) continue;
      if(index.chd_lba < 0 || track.subchannel == Subchannel::None) return {};
      auto frame = readFrame(sector - index.lba + index.chd_lba);
      if(!frame) return {};
      std::vector<u8> output(96);
      const u8* source = frame + 2352;
      if(track.subchannel == Subchannel::RW) {
        std::copy(source, source + 96, output.data());
      } else {
        // RW_RAW stores one bit of P..W in each symbol; expose eight planar 12-byte channels.
        for(u32 symbol : range(96)) {
          for(u32 channel : range(8)) {
            output[channel * 12 + symbol / 8] |= ((source[symbol] >> (7 - channel)) & 1) << (7 - symbol % 8);
          }
        }
      }
      return output;
    }
  }
  return {};
}

inline auto CHD::read(u32 sector) const -> std::vector<u8> {
  // Convert LBA in CD-ROM to LBA in CHD
  for(auto& track : tracks) {
    for(auto& index : track.indices) {
      if (sector >= index.lba && sector <= index.end) {
        if (index.chd_lba < 0) return {};
        auto chd_lba = (sector - index.lba) + index.chd_lba;

        std::vector<u8> output;
        output.resize(track.type == "MODE1" ? 2048 : 2352);

        auto frame = readFrame(chd_lba);
        if(!frame) return {};

        // Audio data is in big-endian, so we need to byteswap
        if (track.type == "AUDIO") {
          const u8* src_ptr = frame;
          u8* dst_ptr = output.data();
          const int value_count = 2352 / sizeof(uint16_t);
          for (int i = 0; i < value_count; i++) {
            u16 value;
            memcpy(&value, src_ptr, sizeof(value));
            value = (value << 8) | (value >> 8);
            memcpy(dst_ptr, &value, sizeof(value));
            src_ptr += sizeof(value);
            dst_ptr += sizeof(value);
          }
        } else {
          std::copy(frame, frame + output.size(), output.data());
        }

        return output;
      }
    }
  }

  print("CHD: Attempting to read from unmapped sector ", sector, "\n");
  return {};
}

inline auto CHD::sectorCount() const -> u32 {
  u32 count = 0;
  for(auto& track : tracks) count += track.sectorCount();
  return count;
}

inline auto CHD::Track::sectorCount() const -> u32 {
  u32 count = 0;
  for(auto& index : indices) count += index.sectorCount();
  return count;
}

inline auto CHD::Index::sectorCount() const -> u32 {
  if(end < 0) return 0;
  return end - lba + 1;
}

}
