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
  if(arguments.size() != 2) return print("usage: mame2bml softwarelist.xml output.bml\n");

  string markupName = arguments.take();
  string outputName = arguments.take();
  if(!markupName.endsWith(".xml")) return print("error: arguments in incorrect order\n");
  if(!outputName.endsWith(".bml")) return print("error: arguments in incorrect order\n");

  string markup = string::read(markupName);
  if(!markup) return print("error: unable to read software list\n");

  if(!output.open(outputName, file::mode::write)) return print("error: unable to write output file\n");

  output.print("database\n");
  output.print("  revision: ", chrono::local::date(), "\n\n");

  pathname = Location::path(markupName);
  auto document = XML::unserialize(markup);

  for(auto header : document) {
    if(header.name() == "softwarelist") {
      for(auto software : header) {
        if(software.name() != "software") continue;
        output.print("game\n");
        output.print("  name:  ", software["name"].string(), "\n");
        output.print("  title: ", software["description"].string(), "\n");

        for(auto sub : software["part"]){
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
