#include <nall/string.hpp>
#include <nall/vector.hpp>
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
    vector<Stream> streams;

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
        media.streams.append(Stream::parse(streamNode));
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
    if(!jsonBuffer) {
      close();
      return false;
    }

    auto jsonString = string((const char*)jsonBuffer.data(), jsonBuffer.size());
    //FIXME: Why is jsonString larger than the input data size when creating from the buffer?
    //HACK: prevent JSON::unserialize from failing due to tailing data
    if(jsonString.length() > jsonBuffer.size()) jsonString = jsonString.slice(0, jsonBuffer.size());

    _mediaInfo = JSON::unserialize(jsonString);
    if(!_mediaInfo) {
      close();
      return false;
    }

    int index = 0;
    int childIndex = 0;
    for(auto media : _mediaInfo["media"]) {
      _media.append(Media::parse(media));
    }

    if(_media.size() == 0) {
      close();
      return false;
    }

    _media.sort([](const Media& a, const Media& b) {
      return a.sequenceNo < b.sequenceNo;
    });

    return true;
  }

  auto close() -> void {
    _archive.close();
    _location = {};
    _mediaInfo = {};
    _media.reset();
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

  auto media() const -> const vector<Media>& {
    return _media;
  }

  auto archive() -> ZIP& {
    return _archive;
  }

private:
  string _location;
  Markup::Node _mediaInfo;
  vector<Media> _media;
  ZIP _archive;
};

}
