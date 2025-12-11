#include <nall/string.hpp>
#include <vector>
#include "nall/string/markup/json.hpp"
#include "nall/string/markup/bml.hpp"
#include "zip.hpp"

namespace nall::Decode {

struct MMI {
  struct Stream {
    string name;
    string type;
    string file;
    string format;
    int channels;
    s64 framesInActiveRegion;
    s64 framesInLeadInRegion;
    s64 framesInLeadOutRegion;

    static Stream parse(Markup::Node node) {
      Stream stream;
      stream.name = node["name"].string();
      stream.type = node["type"].string();
      stream.file = node["file"].string();
      stream.format = node["format"].string();
      stream.channels = node["channels"].integer();
      stream.framesInActiveRegion = node["framesInActiveRegion"].integer();
      stream.framesInLeadInRegion = node["framesInLeadInRegion"].integer();
      stream.framesInLeadOutRegion = node["framesInLeadOutRegion"].integer();
      return stream;
    }
  };

  struct Media {
    string name;
    string type;
    string format;
    int sequenceNo;
    int volumeNo;
    int sideNo;
    string physicalType;
    string masterReference;
    std::vector<Stream> streams;

    static Media parse(Markup::Node node) {
      Media media;
      media.name = node["name"].string();
      media.type = node["type"].string();
      media.format = node["format"].string();
      media.sequenceNo = node["sequenceNo"].integer();
      media.volumeNo = node["volumeNo"].integer();
      media.sideNo = node["sideNo"].integer();
      media.physicalType = node["physicalType"].string();
      media.masterReference = node["masterReference"].string();
      for(auto streamNode : node["streams"]) {
        media.streams.push_back(Stream::parse(streamNode));
      }
      return media;
    }
  };

  auto open(const string& filename) -> bool {
    close();
    if(!_archive.open(filename)) return false;
    _location = filename;
    auto file = _archive.findFile("MediaInfo.json");
    if(!file) {
      close();
      return false;
    }

    auto jsonBuffer = _archive.extract(file.get());
    if(jsonBuffer.empty()) {
      close();
      return false;
    }

    // Currently (2025-08-14) there is no way to construct a nall::string from a fixed-length buffer using
    // nall::string_view, as the variadic constructor overrides "string_view(const char* data, u32 size)",
    // meaning we can't create a string_view from a fixed-length input. We use a std::span here as a
    // workaround.
    auto jsonBufferAsSpan = std::span<const u8>(jsonBuffer.data(), jsonBuffer.size());
    auto jsonString = string(jsonBufferAsSpan);

    _mediaInfo = JSON::unserialize(jsonString);
    if(!_mediaInfo) {
      close();
      return false;
    }

    int index = 0;
    int childIndex = 0;
    for(auto media : _mediaInfo["media"]) {
      _media.push_back(Media::parse(media));
    }

    if(_media.size() == 0) {
      close();
      return false;
    }

    std::ranges::sort(_media, [](const Media& a, const Media& b) {
      return a.sequenceNo < b.sequenceNo;
    });

    return true;
  }

  auto close() -> void {
    _archive.close();
    _location = {};
    _mediaInfo = {};
    _media.clear();
  }

  auto location() const -> string {
    return _location;
  }

  auto manifest() const -> string {
    if(!_mediaInfo) return {};
    auto document = BML::serialize(_mediaInfo);
    if(!document) return {};
    return document;
  }

  auto mediaInfo() const -> Markup::Node {
    return _mediaInfo;
  }

  auto name() -> string {
    if(!_mediaInfo) return {};
    return _mediaInfo["name"].string();
  }

  auto system() -> string {
    if(!_mediaInfo) return {};
    return _mediaInfo["system"].string();
  }

  auto region() -> string {
    if(!_mediaInfo) return {};
    return _mediaInfo["regionCode"].string();
  }

  auto catalogId() -> string {
    if(!_mediaInfo) return {};
    return _mediaInfo["catalogId"].string();
  }

  auto media() -> const std::vector<Media>& {
    return _media;
  }

  auto archive() -> ZIP& {
    return _archive;
  }

private:
  string _location;
  Markup::Node _mediaInfo;
  std::vector<Media> _media;
  ZIP _archive;
};

}
