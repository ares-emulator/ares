#include <nall/nall.hpp>

using namespace nall;

struct Mame2BML {
  auto main(Arguments arguments) -> void;
  auto parse(Markup::Node&) -> void;

private:
  string pathname;
  file_buffer output;
};

auto Mame2BML::main(Arguments arguments) -> void {
  if(arguments.size() < 2) {
    return print("usage: mame2bml softwarelist.xml output.bml ares_core - convert a mame software list xml to bml\n"
                 "       mame2bml machinelist.xml output.bml mame_driver - convert a mame machine list xml to bml\n");
  }

  string markupName = arguments.take();
  string outputName = arguments.take();
  vector<string> driverNames = {};
  if(!markupName.endsWith(".xml")) return print("error: arguments in incorrect order\n");
  if(!outputName.endsWith(".bml")) return print("error: arguments in incorrect order\n");

  while(arguments.size()) {
    string driverName = arguments.take();
    driverNames.append(driverName);
  }

  string markup = string::read(markupName);
  if(!markup) return print("error: unable to read software list\n");

  if(!output.open(outputName, file::mode::write)) return print("error: unable to write output file\n");

  output.print("database\n");
  output.print("  revision: ", chrono::local::date(), "\n");
  output.print("  drivers: ");
  for(auto driver : driverNames) {
    output.print(driver, ", ");
  }
  output.print("\n");

  pathname = Location::path(markupName);
  auto document = XML::unserialize(markup);

  for(auto header : document) {
    // machine list xml (from mame.exe -listxml)
    if(header.name() == "mame") {
      output.print("  romset: ", header["build"].string(), "\n");

      for(auto machine : header) {
        if(machine.name() != "machine") continue;
        string driverName = machine["sourcefile"].string();
        if(!driverNames.find(driverName)) continue;
        string type = "game";
        string IsBIOS = machine["isbios"].string();
        string parent = machine["romof"].string();
        if(IsBIOS == "yes") type = "bios";

        print("found ", type, ": ", machine["name"].string(), " (", machine["description"].string(), ")\n");

        output.print("game\n");
        output.print("  name:    ", machine["name"].string(), "\n");
        output.print("  title:   ", machine["description"].string(), "\n");
        output.print("  board:   ", driverName.trimRight(".cpp"), "\n");
        output.print("  type:    ", type, "\n");
        if(parent) output.print("  parent:  ", parent, "\n");

        string region = "";
        for(auto rom : machine) {
          if(rom.name() == "rom") {
            if(rom["region"].string() != region) {
              region = rom["region"].string().replace(":", "");
              output.print("  ", region, "\n");
            }

            if(rom.name() != "rom") continue;
            output.print("    rom\n");
            if(rom["name"])
              output.print("      name:   ", rom["name"].string(), "\n");
            if(rom["value"])
              output.print("      value:  ", rom["value"].natural(), "\n");
              output.print("      offset: ", "0x", rom["offset"].string(), "\n");
              output.print("      size:   ", rom["size"].natural(), "\n");
              output.print("      crc:    ", rom["crc"].string(), "\n");
              output.print("      sha1:   ", rom["sha1"].string(), "\n");
          }
        }
      }
    }

    // software list xml (from mame/hash/*.xml)
    if(header.name() == "softwarelist") {
      for(auto software : header) {
        if(software.name() != "software") continue;
        output.print("game\n");
        output.print("  name:  ", software["name"].string(), "\n");
        output.print("  title: ", software["description"].string(), "\n");
        output.print("  board: ", driverNames.first(), "\n");

        for(auto sub : software["part"]){
          if(sub.name() == "feature") {
            output.print("  feature\n");
            output.print("    name:  ", sub["name"].string(), "\n");
            output.print("    value: ", sub["value"].string(), "\n");
            continue;
          }

          if(sub.name() != "dataarea") continue;
          output.print("  ", sub["name"].string().replace(":", ""), "\n");
          output.print("    size: ", sub["size"].natural(), "\n");

          for(auto rom : sub) {
            if(rom.name() != "rom") continue;
            output.print("    rom\n");
          if(rom["name"])
            output.print("      name:   ", rom["name"].string(), "\n");
          if(rom["loadflag"])
            output.print("      type:   ", rom["loadflag"].string(), "\n");
          if(rom["value"])
            output.print("      value:  ", rom["value"].natural(), "\n");
            output.print("      offset: ", rom["offset"].natural(), "\n");
            output.print("      size:   ", rom["size"].natural(), "\n");
            output.print("      crc:    ", rom["crc"].string(), "\n");
            output.print("      sha1:   ", rom["sha1"].string(), "\n");
          }
        }

        output.print("\n");
      }
    }
  }
}

#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
  Mame2BML().main(arguments);
}
