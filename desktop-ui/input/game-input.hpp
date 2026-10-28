#pragma once

#include <regex>

namespace GameInput {

struct MediaTitle {
  string title;
  bool grouped = false;
};

//Remove Disc/Disk/Side/Tape labels, including any disc name.
inline auto mediaTitle(const string& title) -> MediaTitle {
  static const std::regex tags{R"([\(\[]\s*(?:Disc|Disk|Side|Tape)(?:\s+|[-:#]\s*|(?=[0-9]))[^\)\]]+[\)\]])", std::regex::icase};
  static const std::regex suffix{
    R"((?:^|\s*[-_:]\s*|\s+)(?:Disc|Disk|Side|Tape)\s*[-:#]?\s*)"
    R"((?:[0-9]+|[A-Z]|[IVX]+)(?:\s*(?:of|/)\s*(?:[0-9]+|[A-Z]|[IVX]+))?)"
    R"((?:\s*[-_:]\s*.*)?\s*$)", std::regex::icase
  };
  static const std::regex whitespace{R"(\s+)"};
  std::string value{title.data(), title.size()};
  bool grouped = std::regex_search(value, tags);
  value = std::regex_replace(value, tags, "");
  while(std::regex_search(value, suffix)) {
    grouped = true;
    value = std::regex_replace(value, suffix, "");
  }
  if(grouped) value = std::regex_replace(value, whitespace, " ");
  return {string{value.c_str()}.strip(), grouped};
}

//Use the game name, not the ROM contents or directory, as the profile identity.
inline auto profileTitle(string location, const string& title) -> string {
  location.transform("\\", "/");
  string parent = Location::dir(location);
  string directory = string{parent}.trimRight("/");
  string directoryName = Location::file(directory);
  auto folder = mediaTitle(directoryName);
  if(folder.grouped) {
    //Disc directories often contain generic filenames such as image.cue.
    if(folder.title) return folder.title;
    string gameDirectory = Location::dir(directory);
    gameDirectory.trimRight("/");
    string gameName = Location::file(gameDirectory);
    return mediaTitle(gameName).title;
  }
  auto metadata = mediaTitle(title);
  if(metadata.title) return metadata.title;
  string filename = Location::prefix(location);
  auto file = mediaTitle(filename);
  if(file.title) return file.title;
  return directoryName;
}

}
