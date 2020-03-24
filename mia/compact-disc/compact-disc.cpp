#include "mega-cd.cpp"
#include "pc-engine-cd.cpp"

auto CompactDisc::construct() -> void {
  Media::construct();
}

auto CompactDisc::manifest(string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  return s;
}

auto CompactDisc::import(string filename) -> string {
  string location = {pathname, Location::prefix(filename), "/"};
  if(!directory::create(location)) return "output directory not writable";

  CD::Session session;
  auto cue = Media::read(filename, ".cue");
  if(!cue) return "cue file missing";
  if(auto error = importCUE(session, string{cue})) return error;

  auto img = Media::read(filename, ".img");
  if(!img) img = Media::read(filename, ".bin");
  if(!img) return "image file missing";
  if(img.size() % 2352) return "image file size not a multiple of 2352";

  session.leadOut.lba = img.size() / 2352 + 150;
  session.synchronize(6750);
  auto sub = session.encode(abs(session.leadIn.lba) + session.leadOut.lba + 6750);

  //overlay an existing subchannel data file, if it exists
  //these files are missing lead-in, track 1 pregap, and lead-out
  if(auto overlay = Media::read(filename, ".sub")) {
    if(overlay.size() == img.size() / 2352 * 96) {
      uint target = (abs(session.leadIn.lba) + 150) * 96;
      memory::copy(sub.data() + target, overlay.data(), overlay.size());
    }
  }

  if(auto fp = file::open({location, "cd.rom"}, file::mode::write)) {
    array_view<uint8_t> imgData{img};
    array_view<uint8_t> subData{sub};

    for(int sector = session.leadIn.lba; sector < session.leadOut.lba + 6750; sector++) {
      if(sector < 150 || sector >= session.leadOut.lba) {
        for(uint index : range(2352)) fp.write(0x00);  //generate missing IMG data
      } else {
        for(uint index : range(2352)) fp.write(*imgData++);
      }
      for(uint index : range(96)) fp.write(*subData++);
    }
  }

  file::write({location, "cd.bml"}, session.serialize());  //for debugging purposes
  return {};
}

auto CompactDisc::importCUE(CD::Session& session, string document) -> string {
  session = {};
  session.leadIn.lba = -7500;

  int track = -1;
  auto lines = document.split("\n").strip();

  for(auto& line : lines) {
    if(line.beginsWith("TRACK ")) {
      line.trimLeft("TRACK ", 1L).strip();
      int control = 0b1111;
      int address = 0b0001;
      if(line.endsWith(" AUDIO")) {
        line.trimRight(" AUDIO", 1L).strip();
        control = 0b0000;
      } else if(line.endsWith(" MODE1/2352")) {
        line.trimRight(" MODE1/2352", 1L).strip();
        control = 0b0100;
      } else return "CUE: unrecognized track type";

      track = line.natural();
      if(track > 99) return "CUE: invalid track number";
      session.tracks[track].control = control;
      session.tracks[track].address = address;
      continue;
    }

    if(line.beginsWith("INDEX ")) {
      if(track == -1) return "CUE: missing track number";
      line.trimLeft("INDEX ", 1L).strip();
      auto part = line.split(" ", 1L);
      if(part.size() != 2) return "CUE: misformatted index";
      int index = part[0].natural();
      if(index > 99) return "CUE: invalid index number";
      auto time = part[1].split(":");
      if(time.size() != 3) return "CUE: misformatted index time";
      auto msf = CD::MSF(time[0].natural(), time[1].natural(), time[2].natural());
      if(!msf) return "CUE: invalid index time";
      msf = CD::MSF::fromLBA(msf.toLBA() + 150);
      if(!msf) return "CUE: invalid index time";
      session.tracks[track].indices[index].lba = msf.toLBA();
      continue;
    }
  }

  for(uint track : range(100)) {
    if(!session.tracks[track]) continue;
    session.firstTrack = track;
    break;
  }

  for(uint track : reverse(range(100))) {
    if(!session.tracks[track]) continue;
    session.lastTrack = track;
    break;
  }

  session.tracks[1].indices[0].lba = 0;
  return {};
}
