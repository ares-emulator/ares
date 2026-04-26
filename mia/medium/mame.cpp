// Helper/Utility for importing roms for systems that are using MAME romsets
struct Mame : Medium {
  auto loadRoms(string location, Markup::Node& info, string sectionName) -> std::vector<u8>;
  auto loadRomFile(string location, string filename, Markup::Node& currentInfo) -> std::vector<u8>;
  auto endianSwap(std::vector<u8>& memory, u32 address = 0, int size = -1) -> void;
};

auto Mame::loadRoms(string location, Markup::Node& info, string sectionName) -> std::vector<u8> {
  std::vector<u8> output;

  if(!openArchive(location)) return output;

  string filename = {};
  std::vector<u8> input = {};
  u32 readOffset = 0;
  string loadType = {};

  for(auto section : info[{"game/", sectionName}]) {
    if(section.name() == "rom") {
      if(section["type"] && section["type"].string() != "continue") loadType = section["type"].string();
      if(section["name"]) {
        filename = section["name"].string().strip();
        if(filename) {
          input = {};
          input = loadRomFile(location, filename, info);
          readOffset = 0;
          if (input.empty()) return output;
        }
      }

      auto writeOffset = section["offset"].natural();
      auto size = section["size"].natural();

      auto startIndex = 0;
      auto increment = 1;
      if(loadType == "load16_byte") {
        increment = 2;
        size *= 2;
        if(writeOffset & 1) {
          startIndex = 1;
          writeOffset--;
        }
      }

      if(output.size() < writeOffset + size) output.resize(writeOffset + size);

      for(auto index = startIndex; index < size; index += increment) {
        if(loadType == "fill") {
          output[index + writeOffset] = section["value"].natural();
          continue;
        }

        output[index + writeOffset] = input[readOffset++];
      }

      if(loadType == "load16_word_swap") endianSwap(output, section["offset"].natural(), size);
    }
  }
  return output;
}

auto Mame::loadRomFile(string location, string filename, Markup::Node& info) -> std::vector<u8> {
  if(auto archive = openArchive(location)) {
    if(auto memory = archiveExtractByName(archive, filename); !memory.empty()) {
      return memory;
    }
  }

  if(auto parent = info["game/parent"].string()) {
    auto manifest = manifestDatabaseArcade(parent);
    auto document = BML::unserialize(manifest);
    if(!document) return {};

    string base = {Location::path(location), "/", document["game/name"].string()};
    for(auto extension : {string{".zip"}, string{".7z"}}) {
      location = {base, extension};
      if(auto memory = loadRomFile(location, filename, document); !memory.empty()) {
        return memory;
      }
    }
  }

  return {};
};


auto Mame::endianSwap(std::vector<u8>& memory, u32 address, int size) -> void {
  if(size < 0) size = (int)memory.size();
  for(u32 index = 0; index < (u32)size; index += 2) {
    swap(memory[address + index + 0], memory[address + index + 1]);
  }
}
